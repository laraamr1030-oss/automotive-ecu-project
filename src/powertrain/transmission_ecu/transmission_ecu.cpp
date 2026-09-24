#include "transmission_ecu.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

TransmissionECU::TransmissionECU() : socket_fd_(-1), running_(false), current_gear_(1) {}

TransmissionECU::~TransmissionECU() {
    shutdown();
}

bool TransmissionECU::initSocketCAN(const std::string& interface_name) {
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "[TRANSMISSION ECU] Failed to create socket\n";
        return false;
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[TRANSMISSION ECU] Failed to get interface index\n";
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[TRANSMISSION ECU] Failed to bind socket\n";
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Listen exclusively to Engine ECU telemetry (CAN ID 0x100)
    struct can_filter rfilter[1];
    rfilter[0].can_id   = 0x100;
    rfilter[0].can_mask = CAN_SFF_MASK;
    setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    std::cout << "[TRANSMISSION ECU] SocketCAN initialized on " << interface_name << " (Listening to 0x100)\n";
    return true;
}

void TransmissionECU::run() {
    if (socket_fd_ < 0) {
        std::cerr << "[TRANSMISSION ECU] Cannot run: Socket not initialized\n";
        return;
    }

    running_ = true;
    std::cout << "[TRANSMISSION ECU] Running inter-ECU processing loop...\n";

    while (running_) {
        struct can_frame frame;
        int nbytes = read(socket_fd_, &frame, sizeof(struct can_frame));

        if (nbytes > 0 && frame.can_id == 0x100) {
            uint16_t rpm = (frame.data[0] << 8) | frame.data[1];

            if (rpm > 3500 && current_gear_ < 6) {
                current_gear_++;
                std::cout << "[TRANSMISSION ECU] Auto Upshift -> Gear " << (int)current_gear_ 
                          << " (Engine RPM: " << rpm << ")\n";
            } else if (rpm < 1500 && current_gear_ > 1) {
                current_gear_--;
                std::cout << "[TRANSMISSION ECU] Auto Downshift -> Gear " << (int)current_gear_ 
                          << " (Engine RPM: " << rpm << ")\n";
            }
        }
        usleep(100000); // 100ms cycle delay
    }
}

 feature/powertrain-and-architecture
void TransmissionECU::requestShutdown() {
    running_ = false;

void TransmissionECU::packTransmissionStatus(uint8_t* buf) const {
   

    uint16_t speed_raw = static_cast<uint16_t>(vehicle_speed_ / 0.01f);
    buf[0] = static_cast<uint8_t>(speed_raw & 0xFF);
    buf[1] = static_cast<uint8_t>((speed_raw >> 8) & 0xFF);
    
    buf[2] = static_cast<uint8_t>(gear_);

    buf[3] = 1; // SpeedValid = true
main
}

void TransmissionECU::shutdown() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        std::cout << "[TRANSMISSION ECU] Socket closed\n";
    }
}