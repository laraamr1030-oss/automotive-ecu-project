#include "bms_ecu.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

BMSEcu::BMSEcu() = default;
BMSEcu::~BMSEcu() { shutdown(); }

void BMSEcu::run() {
    state_ = ECUState::INITIALIZING;
    std::cout << "[BMSEcu] Initializing Battery Management ECU...\n";

    if (!init()) {
        state_ = ECUState::FAULT;
        std::cerr << "[BMSEcu] Initialization failed.\n";
        return;
    }

    state_ = ECUState::RUNNING;
    std::cout << "[BMSEcu] Running. Broadcasting BMS status on vcan0 every 100ms.\n";

    while (state_ == ECUState::RUNNING && !shutdown_requested_) {
        cyclicTask();
        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLIC_PERIOD_MS));
    }

    state_ = ECUState::SHUTTING_DOWN;
    shutdown();
    std::cout << "[BMSEcu] Shutdown complete.\n";
}

void BMSEcu::requestShutdown() {
    shutdown_requested_ = true;
}

bool BMSEcu::init() {
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

void BMSEcu::cyclicTask() {
    // Simulate gradual discharge or temperature fluctuation
    soc_ -= 0.01f;
    soc_ = std::clamp(soc_, 0.0f, 100.0f);

    pack_temp_ += 0.02f;
    pack_temp_ = std::clamp(pack_temp_, -40.0f, 85.0f);

    uint8_t buf[8] = {};
    packBmsStatus(buf);

    struct can_frame frame{};
    frame.can_id  = BMS_STATUS_ID;
    frame.can_dlc = 8;
    std::memcpy(frame.data, buf, 8);

    if (write(can_socket_, &frame, sizeof(frame)) < 0) {
        perror("write");
        state_ = ECUState::FAULT;
    }

    std::cout << "[BMSEcu] SoC=" << soc_ << "% Temp=" << pack_temp_ 
              << "°C Voltage=" << pack_voltage_ << "V\n";
}

void BMSEcu::packBmsStatus(uint8_t* buf) const {
    // Encoding using scale/factor and offset
    uint16_t soc_raw     = static_cast<uint16_t>(soc_ / 0.4f);          // Factor 0.4
    uint8_t  temp_raw    = static_cast<uint8_t>((pack_temp_ + 40.0f) / 0.5f); // Offset -40, Factor 0.5
    uint16_t voltage_raw = static_cast<uint16_t>(pack_voltage_ / 0.1f); // Factor 0.1

    buf[0] = soc_raw & 0xFF;
    buf[1] = (soc_raw >> 8) & 0xFF;
    buf[2] = temp_raw;
    buf[3] = charging_state_;
    buf[4] = voltage_raw & 0xFF;
    buf[5] = (voltage_raw >> 8) & 0xFF;
    buf[6] = 0;
    buf[7] = 0;
}

void BMSEcu::shutdown() {
    if (can_socket_ >= 0) {
        close(can_socket_);
        can_socket_ = -1;
    }
}
