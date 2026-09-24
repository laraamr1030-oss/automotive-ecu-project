#include "bcm.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

BCM::BCM()  = default;
BCM::~BCM() { shutdown(); }

void BCM::run() {
    ecu_state_ = ECUState::INITIALIZING;
    if (!init()) { ecu_state_ = ECUState::FAULT; return; }
    ecu_state_ = ECUState::RUNNING;
    std::cout << "[BCM] Running.\n";

    while (ecu_state_ == ECUState::RUNNING && !shutdown_requested_) {
        cyclicTask();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    shutdown();
}

void BCM::requestShutdown() { shutdown_requested_ = true; }

bool BCM::init() {
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

void BCM::cyclicTask() {
    runScenario();

    uint8_t buf[8] = {};
    
packBCMStatus(buf);

    struct can_frame frame{};
    frame.can_id  = BCM_STATUS_ID;
    frame.can_dlc = 8;
    std::memcpy(frame.data, buf, 8);
    write(can_socket_, &frame, sizeof(frame));

    std::cout << "[BCM] DoorFL=" << state_.door_fl
              << " TurnRight=" << state_.turn_right << "\n";
}

void BCM::runScenario() {
    scenario_tick_++;
    if (scenario_tick_ >= 120) scenario_tick_ = 0;

    state_.door_fl    = (scenario_tick_ >= 50 && scenario_tick_ < 70);
    state_.turn_right = (scenario_tick_ >= 90 && scenario_tick_ < 120);
    state_.ignition   = true;
}

void BCM::packBCMStatus(uint8_t* buf) const {
    uint8_t b0 = 0;
    if (state_.door_fl)  b0 |= (1 << 0);
    if (state_.door_fr)  b0 |= (1 << 1);
    if (state_.door_rl)  b0 |= (1 << 2);
    if (state_.door_rr)  b0 |= (1 << 3);
   
    buf[0] = b0;

    uint8_t b1 = 0;
    if (state_.ignition)   b1 |= (1 << 0);
    if (state_.turn_left)  b1 |= (1 << 1);
    if (state_.turn_right) b1 |= (1 << 2);
    if (state_.hazard)   b1 |= (1 << 3);
    buf[1] = b1;

    buf[2] = static_cast<uint8_t>(state_.battery_volt / 0.1f);
}

void BCM::shutdown() {
    if (can_socket_ >= 0) { close(can_socket_); can_socket_ = -1; }
}
