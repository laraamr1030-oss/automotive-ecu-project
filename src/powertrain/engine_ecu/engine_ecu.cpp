 feature/powertrain-and-architecture
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

#include "engine_ecu.hpp"
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <cstring>

EngineECU::EngineECU() = default;

EngineECU::~EngineECU() {
    shutdown();
}

bool EngineECU::init() {
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return false;

    struct ifreq ifr;
    std::strcpy(ifr.ifr_name, "vcan0");
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) return false;

    struct sockaddr_can addr{}; 
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
 main

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) return false;
    return true;
}

 feature/powertrain-and-architecture
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

void EngineECU::run() {
    std::cout << "[EngineECU] Running transmission loop...\n";
    
    while (true) {
        struct can_frame frame;
        
        // Fix 1: CAN ID set to 0x0C0
        frame.can_id = 0x0C0;
        frame.can_dlc = 8;

        uint16_t raw_rpm = 3000; 

        // Fix 2: Apply x0.25 scaling factor
        uint16_t scaled_rpm = static_cast<uint16_t>(raw_rpm * 0.25);

        for(int i = 0; i < 8; ++i) frame.data[i] = 0;

        // Fix 3 & 4: Little-endian packing into bytes 0 and 1
        frame.data[0] = static_cast<uint8_t>(scaled_rpm & 0xFF);        
        frame.data[1] = static_cast<uint8_t>((scaled_rpm >> 8) & 0xFF); 

        write(s, &frame, sizeof(struct can_frame));
        usleep(100000); 
    }
}

void EngineECU::requestShutdown() {
    shutdown();
}

void EngineECU::shutdown() {
    if (s >= 0) {
        close(s);
        s = -1;
 main
    }

    std::cout << "Engine ECU shut down safely." << std::endl;
}