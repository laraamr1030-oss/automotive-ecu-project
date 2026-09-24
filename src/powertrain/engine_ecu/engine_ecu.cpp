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

       // Dynamic simulation tick (or use your scenario counter)
    static int sim_tick = 0;
    sim_tick = (sim_tick + 1) % 100;
    
    // Unscaled raw RPM value (e.g., swinging between 2000 and 4000 RPM equivalent)
    // Since CANWorker multiplies by 0.25, we divide our target value by 0.25 to send the correct raw byte
    uint16_t target_rpm = 2000 + (sim_tick * 20);
    uint16_t raw_sent_val = static_cast<uint16_t>(target_rpm / 0.25f); 

    // Coolant temp simulation (e.g., starting at 90°C)
    uint8_t coolant_temp = static_cast<uint8_t>(90 + (sim_tick % 5));

    for(int i = 0; i < 8; ++i) frame.data[i] = 0;

    // Little-endian packing: RPM into bytes 0-1, Coolant into byte 2
    frame.data[0] = static_cast<uint8_t>(raw_sent_val & 0xFF);
    frame.data[1] = static_cast<uint8_t>((raw_sent_val >> 8) & 0xFF);
    frame.data[2] = coolant_temp; 

        

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
