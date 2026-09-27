#include "engine_ecu.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

EngineECU::EngineECU()
    :uds_handler_(rpm_, coolant_temp_, vehicle_speed_, battery_voltage_, dtc_manager_) {}

EngineECU::~EngineECU() { shutdown(); }

void EngineECU::run() {
    state_ = ECUState::INITIALIZING;
    std::cout << "[EngineECU] Initializing...\n";

    if (!init()) {
        state_ = ECUState::FAULT;
        std::cerr << "[EngineECU] Initialization failed.\n";
        return;
    }

    state_ = ECUState::RUNNING;
    std::cout << "[EngineECU] Running with UDS + fault simulation on vcan0.\n";

    while (state_ == ECUState::RUNNING && !shutdown_requested_) {
        cyclicTask();

        if (uds_handler_.consumeResetRequest()) {
            std::cout << "[EngineECU] ECU reset requested via UDS -- restarting simulation.\n";
            rpm_             = 0.0f;
            coolant_temp_    = 10.0f;
            battery_voltage_ = 14.2f;
            tick_count_      = 0;
            dtc_manager_.clearAll();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLIC_PERIOD_MS));
    }

    state_ = ECUState::SHUTTING_DOWN;
    shutdown();
    std::cout << "[EngineECU] Shutdown complete.\n";
}

void EngineECU::requestShutdown() {
    shutdown_requested_ = true;
}

bool EngineECU::init() {
    can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket_ < 0) { perror("socket"); return false; }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ);
    if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl"); close(can_socket_); can_socket_ = -1; return false;
    }

    struct sockaddr_can addr{};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(can_socket_); can_socket_ = -1; return false;
    }

    return true;
}

void EngineECU::cyclicTask() {
    ++tick_count_;
    double elapsed_sec = tick_count_ * (CYCLIC_PERIOD_MS / 1000.0);
    double phase_sec   = std::fmod(elapsed_sec, CYCLE_PERIOD_SEC);

    // --- Scripted demo drive cycle -----------------------------------
    // Repeats every CYCLE_PERIOD_SEC so the dashboard has something to
    // react to indefinitely: idle (cold) -> cruise -> misfire burst ->
    // overheat -> voltage spike -> recovery -> back to idle.
    bool inject_misfire  = false;
    bool boost_overheat  = false;
    bool boost_cooling   = false;
    float voltage_target = 14.2f;

    if (phase_sec < 8.0) {
        target_rpm_ = 800.0f;                 // idle, engine still cold
    } else if (phase_sec < 20.0) {
        target_rpm_ = 2200.0f;                // cruise, warming normally
    } else if (phase_sec < 25.0) {
        target_rpm_ = 2200.0f;
        inject_misfire = true;                // P0300 window
    } else if (phase_sec < 40.0) {
        target_rpm_ = 2200.0f;
        boost_overheat = true;                // P0117 window
    } else if (phase_sec < 45.0) {
        target_rpm_ = 2200.0f;
        voltage_target = 16.0f;               // P0563 window
    } else {
        target_rpm_ = 1200.0f;
        boost_cooling = true;                 // recovery before next loop
    }

    // --- RPM ---
    rpm_ += (target_rpm_ - rpm_) * 0.05f;
    if (inject_misfire) {
        // Random oscillation matching the fault table's trigger condition
        // (content.md Section 3.1: "rpm_ oscillates +/- 200"). Magnitude is
        // drawn from [220, 300] -- strictly above the 180 RPM detection
        // threshold in evaluateFaults() -- so every evaluation tick inside
        // this window reliably reads as a failure (a plain +/-200 range
        // would only ever *just* reach the old 15%-of-target=330 threshold
        // and could never exceed it).
        float magnitude = 220.0f + static_cast<float>(std::rand() % 81); // 220..300
        float sign      = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
        rpm_ += sign * magnitude;
    }
    rpm_ = std::max(0.0f, rpm_);
    misfire_window_ = inject_misfire;

    // --- Coolant temperature ---
    if (rpm_ > 1000.0f) coolant_temp_ += 0.05f;
    else                 coolant_temp_ -= 0.02f;
    // +1.0C/tick on top of the +0.05C/tick base warming: measured live-run
    // behavior showed coolant is only ~17C by the time this window starts
    // (t=25s), so the previous +0.4C/tick boost topped out around 87C over
    // the 15s window and never reached the 105C threshold at all. +1.0C/tick
    // crosses 105C around 8-9s into the window, leaving several 2s
    // monitoring cycles to confirm before the window ends at t=40s.
    if (boost_overheat) coolant_temp_ += 1.0f;
    if (boost_cooling)  coolant_temp_ -= 0.5f;  // radiator fan brings it back down
    coolant_temp_ = std::clamp(coolant_temp_, -40.0f, 120.0f);

    // --- Battery voltage ---
    battery_voltage_ += (voltage_target - battery_voltage_) * 0.15f;

    // --- Evaluate fault conditions once per monitoring cycle ---
    if (tick_count_ % MONITORING_CYCLE_TICKS == 0) {
        evaluateFaults(elapsed_sec, phase_sec);
    }

    // --- Broadcast engine status (0x0C0), every cyclic tick ---
    uint8_t status_buf[8] = {};
    packEngineStatus(status_buf);
    struct can_frame status_frame{};
    status_frame.can_id  = ENGINE_STATUS_ID;
    status_frame.can_dlc = 8;
    std::memcpy(status_frame.data, status_buf, 8);
    if (write(can_socket_, &status_frame, sizeof(status_frame)) < 0) {
        perror("write");
        state_ = ECUState::FAULT;
        return;
    }

    // --- Broadcast fault status (0x0C1), every cyclic tick ---
    uint8_t fault_buf[8] = {};
    packFaultStatus(fault_buf);
    struct can_frame fault_frame{};
    fault_frame.can_id  = FAULT_STATUS_ID;
    fault_frame.can_dlc = 2;
    std::memcpy(fault_frame.data, fault_buf, 2);
    if (write(can_socket_, &fault_frame, sizeof(fault_frame)) < 0) {
        perror("write");
        state_ = ECUState::FAULT;
        return;
    }

    std::cout << "[EngineECU] RPM=" << static_cast<int>(rpm_)
              << " Temp=" << coolant_temp_ << "\xC2\xB0" << "C"
              << " Volt=" << battery_voltage_
              << " ActiveDTCs=" << dtc_manager_.getActiveDTCs().size()
              << " CEL=" << (dtc_manager_.hasWarning() ? "ON" : "off") << "\n";

    // Poll for incoming UDS requests without blocking the cyclic task
    // (Session 13, Section 2.1). Drain the *entire* current queue on
    // can_socket_ each tick, not just one frame -- this same raw socket
    // also receives every other frame on the bus (this ECU's own
    // 0x0C0/0x0C1 loopback, transmission_ecu's 0x0D0, bcm's 0x320), so with
    // all 3 other ECUs running that's ~4 unrelated frames queued per 100ms
    // tick. Reading only one per tick let that backlog grow without bound,
    // so a real incoming UDS request frame could sit unprocessed for many
    // seconds -- well past uds_tester.py's 2s ISO-TP timeout, making it
    // appear to hang/fail even though the ECU was running fine.
    //
    // Safe with isoTpSend()'s multi-frame path: handleRequest() ->
    // isoTpSend() -> waitForFrame() does its own blocking read() for the
    // tester's Flow Control frame and fully consumes it *before* control
    // returns here, so this loop never races it away.
    struct can_frame incoming_frame{};
    ssize_t nbytes;

while ((nbytes = recv(can_socket_, &incoming_frame,sizeof(incoming_frame), MSG_DONTWAIT)) > 0) {

    if (incoming_frame.can_id == UDS_REQUEST_ID) {
        uds_handler_.handleRequest(incoming_frame, can_socket_);
    }
    else if (incoming_frame.can_id == 0x0D0) {
        // TransmissionStatus:
        // byte 0     = Current Gear
        // bytes 1-2  = Vehicle Speed raw
        // scale      = 0.01 km/h
        uint16_t speed_raw =
            static_cast<uint16_t>(
                incoming_frame.data[1] |
                (incoming_frame.data[2] << 8)
            );

        vehicle_speed_ = speed_raw * 0.01f;
    }
}
}

void EngineECU::evaluateFaults(double elapsed_sec, double phase_sec) {
    overheat_active_     = coolant_temp_ > 105.0f;                              // P0117
    // Gated on the scripted injection window (not just raw deviation --
    // see engine_ecu.h's misfire_window_ comment) so ordinary post-target-
    // change smoothing lag is never mistaken for a genuine misfire.
    // Fixed 180 RPM threshold (not 15% of target) -- see cyclicTask()'s
    // injected-noise comment: the noise magnitude is always >=220 RPM, so
    // this is comfortably and reliably exceeded on every evaluation tick
    // inside the injection window.
    misfire_active_ = misfire_window_ &&
                       std::fabs(rpm_ - target_rpm_) > 180.0f; // P0300
    voltage_high_active_ = battery_voltage_ > 15.5f;                            // P0563
    // P0128: coolant still cold 5 (accelerated) minutes after start -- a
    // one-time-per-process check, not re-armed on every drive-cycle loop
    // (see engine_ecu.h's WARMUP_CHECK_SEC comment).
    warmup_active_ = (elapsed_sec >= WARMUP_CHECK_SEC) &&
                      (elapsed_sec < WARMUP_CHECK_SEC + CYCLE_PERIOD_SEC) &&
                      (coolant_temp_ < 15.0f);

    // Snapshot of "key sensor values" (content.md's freeze-frame learning
    // objective) as of this monitoring-cycle evaluation -- FaultManager
    // only actually stores it the first time a given DTC is set.
    FreezeFrame snapshot{rpm_, coolant_temp_, battery_voltage_, false};

    dtc_manager_.evaluate(0x01, 0x17, overheat_active_, snapshot);
    // content.md Section 3.1's fault table gives P0300 as (0x30, 0x00)
    // verbatim -- kept as-is for fidelity to the source table, even though
    // that byte pair doesn't reduce back to "P0300" under the same
    // section's own bits7-6/5-4/3-0 encoding formula (Section 1.2), unlike
    // the other three table entries, which all decode correctly.
    dtc_manager_.evaluate(0x30, 0x00, misfire_active_, snapshot);
    dtc_manager_.evaluate(0x05, 0x63, voltage_high_active_, snapshot);
    dtc_manager_.evaluate(0x01, 0x28, warmup_active_, snapshot);

    (void)phase_sec;
}

void EngineECU::packEngineStatus(uint8_t* buf) const {
    uint16_t rpm_raw  = static_cast<uint16_t>(rpm_ / 0.25f);
    // factor=1.0, offset=-40 -> range -40..215C, one degree per count.
    // Session 11's original 0.5 factor only covers up to 87.5C, which this
    // demo's overheat window deliberately exceeds (clamped up to 120C),
    // silently corrupting the broadcast temperature via the out-of-range
    // float->uint8_t cast -- see CANWorker.cpp's and uds_handler.cpp's
    // matching decode/encode, which must stay in sync with this factor.
    uint8_t  temp_raw = static_cast<uint8_t>(coolant_temp_ + 40.0f);
    uint8_t  thr_raw  = static_cast<uint8_t>(throttle_ / 0.4f);
    uint8_t  running  = (rpm_ > 0.0f) ? 1 : 0;

    buf[0] = rpm_raw & 0xFF;
    buf[1] = (rpm_raw >> 8) & 0xFF;
    buf[2] = 0; buf[3] = 0;
    buf[4] = temp_raw;
    buf[5] = thr_raw;
    buf[6] = running;
    buf[7] = 0;
}

void EngineECU::packFaultStatus(uint8_t* buf) const {
    // content.md Section 3.4, extended with bits 2-3 for this demo's extra
    // two fault conditions (the pdf's worked example only shows bits 0-1).
    buf[0] = 0;
    if (overheat_active_)     buf[0] |= 0x01; // bit0: overheating (P0117)
    if (misfire_active_)      buf[0] |= 0x02; // bit1: misfire (P0300)
    if (voltage_high_active_) buf[0] |= 0x04; // bit2: voltage high (P0563)
    if (warmup_active_)       buf[0] |= 0x08; // bit3: coolant cold after start (P0128)
    buf[1] = static_cast<uint8_t>(dtc_manager_.getActiveDTCs().size());
}

void EngineECU::shutdown() {
    if (can_socket_ >= 0) {
        close(can_socket_);
        can_socket_ = -1;
    }
}
