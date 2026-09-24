#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <cstring>

class EngineECU {
public:
    EngineECU() = default;
    ~EngineECU() { shutdown(); }

    bool init() {
        // Setup socketcan vcan0
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

    void run() {
        std::cout << "[EngineECU] Running transmission loop...\n";
        
        while (true) {
            struct can_frame frame;
            
            // Set CAN ID to 0x0C0 (matching signal dictionary)
            frame.can_id = 0x0C0;
            frame.can_dlc = 8;

            // Raw sensor value example
            static uint16_t raw_rpm = 1000; 
            raw_rpm+= 100;
            if (raw_rpm > 4000) raw_rpm = 1000; // Loop it back down


            // Apply the x0.25 scaling factor
            uint16_t scaled_rpm = static_cast<uint16_t>(raw_rpm * 0.25);

            // Clear payload bytes
            for(int i = 0; i < 8; ++i) frame.data[i] = 0;

            // Pack into bytes 0 to 1 using little-endian byte order
            frame.data[0] = static_cast<uint8_t>(raw_rpm & 0xFF);        // Byte 0: LSB
            frame.data[1] = static_cast<uint8_t>((raw_rpm >> 8) & 0xFF); // Byte 1: MSB

            write(s, &frame, sizeof(struct can_frame));
            usleep(100000); // Send every 100ms
        }
    }

    void shutdown() {
        if (s >= 0) close(s);
    }

private:
    int s = -1;
};

int main() {
    EngineECU ecu;
    if (!ecu.init()) {
        std::cerr << "[EngineECU] Initialization failed.\n";
        return 1;
    }
    ecu.run();
    return 0;
}
