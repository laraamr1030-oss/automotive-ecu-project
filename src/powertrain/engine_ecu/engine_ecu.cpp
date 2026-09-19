#include "engine_ecu.h"

#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

EngineECU::EngineECU()  = default;
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
    std::cout << "[EngineECU] Running. Broadcasting on vcan0 every 100ms.\n";

    while (state_ == ECUState::RUNNING && !shutdown_requested_) {
        cyclicTask();
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
    if (demo_mode_) {
        // Continuously re-target rpm_ along a repeating idle<->rev sweep
        // instead of letting it settle on a fixed target_rpm_ and go static.
        ++demo_tick_;
        long ms_in_cycle = (static_cast<long>(demo_tick_) * CYCLIC_PERIOD_MS) % DEMO_CYCLE_MS;
        constexpr float TWO_PI = 6.283185307f;
        float phase = TWO_PI * static_cast<float>(ms_in_cycle) / static_cast<float>(DEMO_CYCLE_MS);
        target_rpm_ = DEMO_IDLE_RPM + (DEMO_PEAK_RPM - DEMO_IDLE_RPM) * (0.5f - 0.5f * std::cos(phase));
        throttle_   = 10.0f + 70.0f * (target_rpm_ - DEMO_IDLE_RPM) / (DEMO_PEAK_RPM - DEMO_IDLE_RPM);
    }

    rpm_ += (target_rpm_ - rpm_) * 0.05f;

    if (rpm_ > 1000.0f) coolant_temp_ += 0.05f;
    else                 coolant_temp_ -= 0.02f;
    coolant_temp_ = std::clamp(coolant_temp_, -40.0f, 120.0f);

    uint8_t buf[8] = {};
    packEngineStatus(buf);

    struct can_frame frame{};
    frame.can_id  = ENGINE_STATUS_ID;
    frame.can_dlc = 8;
    std::memcpy(frame.data, buf, 8);

    if (write(can_socket_, &frame, sizeof(frame)) < 0) {
        perror("write");
        state_ = ECUState::FAULT;
    }

    std::cout << "[EngineECU] RPM=" << static_cast<int>(rpm_)
              << " Temp=" << coolant_temp_ << "°C\n";
}

void EngineECU::packEngineStatus(uint8_t* buf) const {
    uint16_t rpm_raw  = static_cast<uint16_t>(rpm_ / 0.25f);
    uint8_t  temp_raw = static_cast<uint8_t>((coolant_temp_ + 40.0f) / 0.5f);
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

void EngineECU::shutdown() {
    if (can_socket_ >= 0) {
        close(can_socket_);
        can_socket_ = -1;
    }
}
