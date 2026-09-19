#include "CANWorker.h"
#include <QDebug>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cstring>

CANWorker::CANWorker(QObject *parent) : QObject(parent) {}

CANWorker::~CANWorker() {
    stop();
}

void CANWorker::process() {
    run();
}

void CANWorker::stop() {
    running_ = false;
    stop_ = true;
    if (can_socket_ >= 0) {
        close(can_socket_);
        can_socket_ = -1;
    }
}

bool CANWorker::openSocket() {
    can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket_ < 0) return false;

    struct ifreq ifr;
    std::strcpy(ifr.ifr_name, "vcan0");
    if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
        close(can_socket_);
        can_socket_ = -1;
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_socket_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(can_socket_);
        can_socket_ = -1;
        return false;
    }

    return true;
}

void CANWorker::run() {
    if (!openSocket()) {
        qWarning() << "Failed to open vcan0 socket";
        return;
    }

    running_ = true;
    stop_ = false;
    struct can_frame frame;

    while (running_ && !stop_) {
        ssize_t nbytes = read(can_socket_, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            break;
        }
        if (nbytes == sizeof(struct can_frame)) {
            decodeAndEmit(frame.can_id, frame.data, frame.can_dlc);
        }
    }
}

void CANWorker::decodeAndEmit(canid_t can_id, const uint8_t *data, uint8_t dlc) {
    if (!data) return;

    // Mask standard frame ID
    uint32_t id = can_id & CAN_SFF_MASK;

    switch (id) {
    case 0x0C0: { // Speed & RPM
        if (dlc >= 4) {
            int speed = data[0] | (data[1] << 8);
            int rpm = data[2] | (data[3] << 8);
            emit vehicleSpeedUpdated(speed);
            emit engineSpeedUpdated(rpm);
        }
        break;
    }
    case 0x0D0: { // Engine Temp, Fuel, Battery
        if (dlc >= 3) {
            float temp = static_cast<float>(data[0]);
            float fuel = static_cast<float>(data[1]);
            float battery = static_cast<float>(data[2]) / 10.0f;
            emit coolantTempUpdated(temp);
            emit fuelLevelUpdated(fuel);
            emit batteryVoltageUpdated(battery);
        }
        break;
    }
    case 0x320: { // Doors & Indicators
        if (dlc >= 2) {
            bool fl = data[0] & 0x01;
            bool fr = data[0] & 0x02;
            bool rl = data[0] & 0x04;
            bool rr = data[0] & 0x08;
            emit doorStatusUpdated(fl, fr, rl, rr);

            bool left = data[1] & 0x01;
            bool right = data[1] & 0x02;
            bool hazard = data[1] & 0x04;
            emit turnSignalsChanged(left, right);
            emit hazardChanged(hazard);
        }
        break;
    }
    default:
        break;
    }
}
