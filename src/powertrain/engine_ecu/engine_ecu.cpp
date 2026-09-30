#include "engine_ecu.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

EngineECU::EngineECU() : uds_handler_(live_, dtc_manager_) {
    resetSimulation();
}

EngineECU::~EngineECU() { shutdown(); }

void EngineECU::resetSimulation() {
    live_.rpm             = 0.0f;
    live_.coolant_temp    = 10.0f;
    live_.battery_voltage = 14.2f;
    live_.throttle        = 0.0f;
    base_rpm_   = 0.0f;
    tick_count_ = 0;
}

void EngineECU::run() {
    state_ = ECUState::INITIALIZING;
    std::cout << "[EngineECU] Initializing...\n";
    if (!init()) {
        state_ = ECUState::FAULT;
        std::cerr << "[EngineECU] Initialization failed (is vcan0 up?).\n";
        return;
    }
    state_ = ECUState::RUNNING;
    std::cout << "[EngineECU] Running: 0x0C0/0x0C1 broadcast, UDS on 0x7E0/0x7E8, vcan0.\n";

    while (state_ == ECUState::RUNNING && !shutdown_requested_) {
        cyclicTask();

        if (uds_handler_.consumeResetRequest()) {
            std::cout << "[EngineECU] ECU reset requested via UDS -- restarting simulation.\n";
            resetSimulation();
            dtc_manager_.clearAll();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLIC_PERIOD_MS));
    }

    state_ = ECUState::SHUTTING_DOWN;
    shutdown();
    std::cout << "[EngineECU] Shutdown complete.\n";
}

void EngineECU::requestShutdown() { shutdown_requested_ = true; }

bool EngineECU::init() {
    can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket_ < 0) { perror("socket"); return false; }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);
    if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl"); close(can_socket_); can_socket_ = -1; return false;
    }

    struct sockaddr_can addr{};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); close(can_socket_); can_socket_ = -1; return false;
    }
    return true;
}

void EngineECU::cyclicTask() {
    ++tick_count_;
    const double elapsed_sec = tick_count_ * (CYCLIC_PERIOD_MS / 1000.0);
    const double phase_sec   = std::fmod(elapsed_sec, CYCLE_PERIOD_SEC);

    // --- Scripted drive cycle, repeats every 60 s -------------------------
    //  0-8 idle | 8-20 cruise | 20-28 misfire | 28-40 overheat |
    //  40-46 12V spike | 46-60 recovery
    bool  inject_misfire = false;
    float coolant_target = 85.0f;   // normal operating temperature
    float voltage_target = 14.2f;

    if (phase_sec < 8.0)        { target_rpm_ = 800.0f;  coolant_target = 80.0f; }
    else if (phase_sec < 20.0)  { target_rpm_ = 2200.0f; }
    else if (phase_sec < 28.0)  { target_rpm_ = 2200.0f; inject_misfire = true; }      // P0300 window
    else if (phase_sec < 40.0)  { target_rpm_ = 2200.0f; coolant_target = 125.0f; }    // P0117 window
    else if (phase_sec < 46.0)  { target_rpm_ = 2200.0f; voltage_target = 16.0f; }     // P0563 window
    else                        { target_rpm_ = 1200.0f; }                             // recovery

    // --- RPM: first-order approach to the target, plus misfire noise ------
    base_rpm_ += (target_rpm_ - base_rpm_) * 0.05f;
    float noise = 0.0f;
    if (inject_misfire) {
        float magnitude = 220.0f + static_cast<float>(std::rand() % 81);   // 220..300
        noise = ((std::rand() % 2) == 0 ? 1.0f : -1.0f) * magnitude;
    }
    // Noise is added to the *reported* RPM only (not accumulated into
    // base_rpm_), so |rpm - target| is >= 220 on every tick of the window.
    live_.rpm = std::max(0.0f, base_rpm_ + noise);
    misfire_window_ = inject_misfire;
    live_.running   = (live_.rpm > 0.0f) ? 1 : 0;
    live_.throttle  = 5.0f + (target_rpm_ - 800.0f) / 50.0f;   // idle 5 %, cruise 33 %

    // --- Coolant and 12 V system: first-order approach to their targets ---
    live_.coolant_temp += (coolant_target - live_.coolant_temp) * 0.02f;
    live_.coolant_temp  = std::clamp(live_.coolant_temp, -40.0f, 150.0f);
    live_.battery_voltage += (voltage_target - live_.battery_voltage) * 0.15f;

    // --- Fault monitoring, once per monitoring cycle (2 s) ----------------
    if (tick_count_ % MONITORING_CYCLE_TICKS == 0) {
        evaluateFaults();
    }

    // --- Publish fault summary for the CAN frames and for UDS -------------
    live_.dtc_count = static_cast<uint8_t>(dtc_manager_.getActiveDTCs().size());
    live_.cel_on    = dtc_manager_.hasWarning() ? 1 : 0;
    live_.fault_flags = 0;
    if (overheat_active_)      live_.fault_flags |= 0x01;
    if (misfire_active_)       live_.fault_flags |= 0x02;
    if (voltage_high_active_)  live_.fault_flags |= 0x04;
    if (pack_overtemp_active_) live_.fault_flags |= 0x08;
    if (pack_volt_low_active_) live_.fault_flags |= 0x10;

    // --- Broadcast 0x0C0 engine status ------------------------------------
    struct can_frame status_frame{};
    status_frame.can_id  = ENGINE_STATUS_ID;
    status_frame.can_dlc = 8;
    packEngineStatus(status_frame.data);
    if (write(can_socket_, &status_frame, sizeof(status_frame)) < 0) {
        perror("write"); state_ = ECUState::FAULT; return;
    }

    // --- Broadcast 0x0C1 fault status (3 bytes) ----------------------------
    struct can_frame fault_frame{};
    fault_frame.can_id  = FAULT_STATUS_ID;
    fault_frame.can_dlc = 3;
    packFaultStatus(fault_frame.data);
    if (write(can_socket_, &fault_frame, sizeof(fault_frame)) < 0) {
        perror("write"); state_ = ECUState::FAULT; return;
    }

    std::cout << "[EngineECU] RPM=" << static_cast<int>(live_.rpm)
              << " Coolant=" << live_.coolant_temp << "C"
              << " 12V=" << live_.battery_voltage
              << " PackT=" << live_.pack_temp << " PackV=" << live_.pack_voltage
              << " DTCs=" << static_cast<int>(live_.dtc_count)
              << " CEL=" << (live_.cel_on ? "ON" : "off") << "\n";

    processIncomingFrames();
}

void EngineECU::processIncomingFrames() {
    // Drain EVERYTHING queued on the socket each tick (the bus carries ~5
    // unrelated frames per tick). recv(MSG_DONTWAIT) never blocks the cycle.
    struct can_frame f{};
    ssize_t n;
    while ((n = recv(can_socket_, &f, sizeof(f), MSG_DONTWAIT)) > 0) {
        if (n < static_cast<ssize_t>(sizeof(f))) continue;
        const uint32_t id = f.can_id & CAN_SFF_MASK;

        switch (id) {
        case UDS_REQUEST_ID:
            uds_handler_.handleRequest(f, can_socket_);
            break;

        case TRANSMISSION_STATUS_ID:   // bytes0-1 speed (LE, 0.01 km/h), byte2 gear
            if (f.can_dlc >= 3) {
                uint16_t raw = static_cast<uint16_t>(f.data[0] | (f.data[1] << 8));
                live_.vehicle_speed = raw * 0.01f;
                live_.gear = f.data[2];
            }
            break;

        case BCM_STATUS_ID:            // kept raw; UDS returns the same bytes
            if (f.can_dlc >= 3) {
                live_.bcm_doors    = f.data[0];
                live_.bcm_lights   = f.data[1];
                live_.bcm_batt_raw = f.data[2];
            }
            break;

        case BMS_STATUS_ID:            // layout = bms_ecu.cpp packBmsStatus()
            if (f.can_dlc >= 6) {
                uint16_t soc_raw  = static_cast<uint16_t>(f.data[0] | (f.data[1] << 8));
                uint16_t volt_raw = static_cast<uint16_t>(f.data[4] | (f.data[5] << 8));
                live_.soc          = soc_raw * 0.4f;
                live_.pack_temp    = f.data[2] * 0.5f - 40.0f;
                live_.charging     = f.data[3];
                live_.pack_voltage = volt_raw * 0.1f;
                bms_seen_ = true;
            }
            break;

        default:
            break;
        }
    }
}

void EngineECU::evaluateFaults() {
    overheat_active_     = live_.coolant_temp > 105.0f;                                  // P0117
    misfire_active_      = misfire_window_ && std::fabs(live_.rpm - target_rpm_) > 180.0f; // P0300
    voltage_high_active_ = live_.battery_voltage > 15.5f;                                // P0563
    pack_overtemp_active_ = bms_seen_ && live_.pack_temp    > 55.0f;                     // P0A7E
    pack_volt_low_active_ = bms_seen_ && live_.pack_voltage < 320.0f;                    // P0AFA

    // Freeze frame = every "key" value at this instant. FaultManager only
    // stores it the FIRST time a given DTC is set.
    FreezeFrame snap;
    snap.rpm             = live_.rpm;
    snap.coolant_temp    = live_.coolant_temp;
    snap.battery_voltage = live_.battery_voltage;
    snap.soc             = live_.soc;
    snap.pack_temp       = live_.pack_temp;
    snap.pack_voltage    = live_.pack_voltage;

    dtc_manager_.evaluate(0x01, 0x17, overheat_active_,      snap);  // P0117 coolant over-temp
    dtc_manager_.evaluate(0x03, 0x00, misfire_active_,       snap);  // P0300 random misfire
    dtc_manager_.evaluate(0x05, 0x63, voltage_high_active_,  snap);  // P0563 system voltage high
    dtc_manager_.evaluate(0x0A, 0x7E, pack_overtemp_active_, snap);  // P0A7E battery pack over-temp
    dtc_manager_.evaluate(0x0A, 0xFA, pack_volt_low_active_, snap);  // P0AFA battery pack voltage low
}

// 0x0C0: bytes0-1 rpm (LE, 0.25) | byte4 coolant (1.0, -40) | byte5 throttle (0.4)
//        byte6 running | byte7 12V system voltage (0.1). Bytes 2-3 reserved.
void EngineECU::packEngineStatus(uint8_t* buf) const {
    uint16_t rpm_raw = static_cast<uint16_t>(std::clamp(live_.rpm / 0.25f, 0.0f, 65535.0f));
    uint8_t  temp_raw = static_cast<uint8_t>(std::clamp(live_.coolant_temp + 40.0f, 0.0f, 255.0f));
    uint8_t  thr_raw  = static_cast<uint8_t>(std::clamp(live_.throttle / 0.4f, 0.0f, 255.0f));
    uint8_t  volt_raw = static_cast<uint8_t>(std::clamp(live_.battery_voltage / 0.1f, 0.0f, 255.0f));

    std::memset(buf, 0, 8);
    buf[0] = rpm_raw & 0xFF;
    buf[1] = (rpm_raw >> 8) & 0xFF;
    buf[4] = temp_raw;
    buf[5] = thr_raw;
    buf[6] = live_.running;
    buf[7] = volt_raw;
}

// 0x0C1: byte0 fault flags | byte1 stored DTC count | byte2 CEL request
void EngineECU::packFaultStatus(uint8_t* buf) const {
    std::memset(buf, 0, 8);
    buf[0] = live_.fault_flags;
    buf[1] = live_.dtc_count;
    buf[2] = live_.cel_on;
}

void EngineECU::shutdown() {
    if (can_socket_ >= 0) { close(can_socket_); can_socket_ = -1; }
}
