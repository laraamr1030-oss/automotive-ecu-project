#include "transmission_ecu.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

TransmissionECU::TransmissionECU()
    : socket_fd_(-1), running_(false), current_gear_(1), rpm_(0.0f), speed_kmh_(0.0f) {}

TransmissionECU::~TransmissionECU() { shutdown(); }

bool TransmissionECU::initSocketCAN(const std::string& interface_name) {
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "[TRANSMISSION ECU] Failed to create socket\n";
        return false;
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[TRANSMISSION ECU] Failed to get interface index (is " << interface_name << " up?)\n";
        close(socket_fd_); socket_fd_ = -1;
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[TRANSMISSION ECU] Failed to bind socket\n";
        close(socket_fd_); socket_fd_ = -1;
        return false;
    }

    // Kernel-side filter: only engine status (0x0C0) is ever queued for us.
    struct can_filter rfilter[1];
    rfilter[0].can_id   = ENGINE_STATUS_ID;
    rfilter[0].can_mask = CAN_SFF_MASK;
    setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    std::cout << "[TRANSMISSION ECU] SocketCAN initialized on " << interface_name
              << " (listens 0x0C0, sends 0x0D0)\n";
    return true;
}

void TransmissionECU::run() {
    if (socket_fd_ < 0 && !initSocketCAN("vcan0")) {   // main.cpp never calls init itself
        std::cerr << "[TRANSMISSION ECU] Cannot run: socket not initialized\n";
        return;
    }
    running_ = true;
    std::cout << "[TRANSMISSION ECU] Running.\n";

    while (running_) {
        cyclicTask();
        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLIC_PERIOD_MS));
    }
}

void TransmissionECU::cyclicTask() {
    // 1) Drain queued engine frames without blocking; keep the newest RPM.
    struct can_frame in{};
    while (recv(socket_fd_, &in, sizeof(in), MSG_DONTWAIT) > 0) {
        if ((in.can_id & CAN_SFF_MASK) == ENGINE_STATUS_ID && in.can_dlc >= 2) {
            uint16_t raw = static_cast<uint16_t>(in.data[0] | (in.data[1] << 8));
            rpm_ = raw * 0.25f;                       // 0x0C0 signal: factor 0.25
        }
    }

    // 2) Simple drivetrain model: speed follows RPM above idle (800 rpm -> 0 km/h,
    //    2200 rpm -> ~90 km/h), smoothed so it accelerates instead of jumping.
    float target_speed = std::max(0.0f, rpm_ - 800.0f) * 0.0643f;
    speed_kmh_ += (target_speed - speed_kmh_) * 0.1f;

    // 3) Gear from speed, with +/-2 km/h hysteresis so it doesn't flutter.
    static const float up_speed[5] = {15.0f, 35.0f, 55.0f, 75.0f, 95.0f};   // gear g -> g+1
    if (current_gear_ < 6 && speed_kmh_ > up_speed[current_gear_ - 1] + 2.0f) {
        ++current_gear_;
        std::cout << "[TRANSMISSION ECU] Upshift -> Gear " << static_cast<int>(current_gear_) << "\n";
    } else if (current_gear_ > 1 && speed_kmh_ < up_speed[current_gear_ - 2] - 2.0f) {
        --current_gear_;
        std::cout << "[TRANSMISSION ECU] Downshift -> Gear " << static_cast<int>(current_gear_) << "\n";
    }

    // 4) Broadcast 0x0D0: bytes0-1 speed (LE, factor 0.01 km/h), byte2 gear.
    uint16_t speed_raw = static_cast<uint16_t>(std::clamp(speed_kmh_ / 0.01f, 0.0f, 65535.0f));
    struct can_frame out{};
    out.can_id  = TRANSMISSION_STATUS_ID;
    out.can_dlc = 8;
    out.data[0] = speed_raw & 0xFF;
    out.data[1] = (speed_raw >> 8) & 0xFF;
    out.data[2] = current_gear_;
    if (write(socket_fd_, &out, sizeof(out)) < 0) {
        std::cerr << "[TRANSMISSION ECU] write failed\n";
    }
    std::cout << "[TRANSMISSION ECU] RPM=" << rpm_ << " Speed=" << speed_kmh_
              << " km/h Gear=" << static_cast<int>(current_gear_) << "\n";
}

void TransmissionECU::requestShutdown() { running_ = false; }

void TransmissionECU::shutdown() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        std::cout << "[TRANSMISSION ECU] Socket closed\n";
    }
}
