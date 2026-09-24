#include "transmission_ecu.h"
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
#include <fcntl.h>

TransmissionECU::TransmissionECU()  = default;
TransmissionECU::~TransmissionECU() { shutdown(); }

void TransmissionECU::run() {
    state_ = ECUState::INITIALIZING;
    if (!init()) { state_ = ECUState::FAULT; return; }
    state_ = ECUState::RUNNING;
    std::cout << "[TransmissionECU] Running.\n";

    while (state_ == ECUState::RUNNING && !shutdown_requested_) {
        cyclicTask();
        std::this_thread::sleep_for(std::chrono::milliseconds(CYCLIC_PERIOD_MS));
    }
    shutdown();
}

void TransmissionECU::requestShutdown() { shutdown_requested_ = true; }

bool TransmissionECU::init() {
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

    // Non-blocking: this ECU both sends and receives on the same socket,
    // and a blocking read() with no frame waiting would stall the whole loop.
    int flags = fcntl(can_socket_, F_GETFL, 0);
    fcntl(can_socket_, F_SETFL, flags | O_NONBLOCK);
    return true;
}

void TransmissionECU::cyclicTask() {
    struct can_frame in{};
    ssize_t n = read(can_socket_, &in, sizeof(in));
    if (n > 0 && in.can_id == ENGINE_STATUS_ID) {
        engine_rpm_    = (in.data[0] | (in.data[1] << 8)) * 0.25f; // matches Engine ECU's encode factor
        vehicle_speed_ = engine_rpm_ * 0.03f;
    }
    // n < 0 with errno EAGAIN/EWOULDBLOCK just means "no frame yet" — not a fault.

    uint8_t buf[8] = {};
    packTransmissionStatus(buf);

    struct can_frame out{};
    out.can_id  = TRANS_STATUS_ID;
    out.can_dlc = 8;
    std::memcpy(out.data, buf, 8);
    if (write(can_socket_, &out, sizeof(out)) < 0) {
        perror("write");
        state_ = ECUState::FAULT;
    }

    std::cout << "[TransmissionECU] RPM=" << engine_rpm_
              << " Speed=" << vehicle_speed_ << " km/h, Gear=D\n";
}

void TransmissionECU::packTransmissionStatus(uint8_t* buf) const {
   

    uint16_t speed_raw = static_cast<uint16_t>(vehicle_speed_ / 0.01f);
    buf[0] = static_cast<uint8_t>(speed_raw & 0xFF);
    buf[1] = static_cast<uint8_t>((speed_raw >> 8) & 0xFF);
    
    buf[2] = static_cast<uint8_t>(gear_);

    buf[3] = 1; // SpeedValid = true
}

void TransmissionECU::shutdown() {
    if (can_socket_ >= 0) { close(can_socket_); can_socket_ = -1; }
}
