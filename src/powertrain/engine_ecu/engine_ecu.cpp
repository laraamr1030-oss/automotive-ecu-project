#include "engine_ecu.h"
#include <iostream>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

EngineECU::EngineECU() : socket_fd_(-1), running_(false), demo_mode_(false) {}

EngineECU::~EngineECU() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

void EngineECU::setDemoMode(bool enable) {
    demo_mode_ = enable;
}

bool EngineECU::initSocketCAN(const std::string& interface_name) {
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "Failed to create CAN socket." << std::endl;
        return false;
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Failed to locate interface: " << interface_name << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind CAN socket to " << interface_name << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    return true;
}

void EngineECU::requestShutdown() {
    running_ = false;
}

void EngineECU::run() {
    if (!initSocketCAN("vcan0")) {
        std::cerr << "Initialization failed. Exiting..." << std::endl;
        return;
    }

    running_ = true;
    std::cout << "Engine ECU running... Broadcasting telemetry on vcan0." << std::endl;

    uint16_t rpm = 800;
    uint8_t temp = 85;

    while (running_) {
        struct can_frame frame;
        frame.can_id = 0x100;
        frame.can_dlc = 8;
        std::memset(frame.data, 0, 8);

        frame.data[0] = (rpm >> 8) & 0xFF;
        frame.data[1] = rpm & 0xFF;
        frame.data[2] = temp;

        if (write(socket_fd_, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            std::cerr << "Error transmitting CAN frame." << std::endl;
        }

        rpm += 50;
        if (rpm > 5500) {
            rpm = 800;
        }

        usleep(100000);
    }

    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    std::cout << "Engine ECU shut down safely." << std::endl;
}