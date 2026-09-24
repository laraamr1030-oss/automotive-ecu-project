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

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) return false;
    return true;
}

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
    }
}
