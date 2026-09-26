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

void CANWorker::simulateFault(bool active) {
    emit faultStatusUpdated(active);
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

    uint32_t id = can_id & CAN_SFF_MASK;

    switch (id) {
    case 0x0C0: { // Engine ECU -- per docs/signal_dictionary.md
        if (dlc >= 3) {
            uint16_t rpm_raw = static_cast<uint16_t>(data[0] | (data[1] << 8));
            emit engineSpeedUpdated(static_cast<int>(rpm_raw * 0.25f));

            float temp = static_cast<float>(data[2]) * 1.0f - 40.0f;
            emit coolantTempUpdated(temp);
        }
        break;
    }
    case 0x0D0: { // Transmission ECU -- per docs/signal_dictionary.md
        if (dlc >= 3) {
            uint16_t speed_raw = static_cast<uint16_t>(data[0] | (data[1] << 8));
            emit vehicleSpeedUpdated(speed_raw * 0.01f);

            emit currentGearUpdated(static_cast<int>(data[2]));
        }
        break;
    }
    case 0x320: { // Body Control Module -- doors + turn signals/hazard
        if (dlc >= 2) {
            bool fl = data[0] & 0x01;
            bool fr = data[0] & 0x02;
            bool rl = data[0] & 0x04;
            bool rr = data[0] & 0x08;
            emit doorStatusUpdated(fl, fr, rl, rr);

            bool left   = data[1] & 0x01;
            bool right  = data[1] & 0x02;
            bool hazard = data[1] & 0x04;
            emit turnSignalsChanged(left, right);
            emit hazardChanged(hazard);
        }
        break;
    }
    case 0x350: { // BMS ECU -- matches bms_ecu.cpp's packBmsStatus() exactly
        if (dlc >= 6) {
            uint16_t soc_raw = static_cast<uint16_t>(data[0] | (data[1] << 8));
            emit socUpdated(soc_raw * 0.4f);

            float packTemp = static_cast<float>(data[2]) * 0.5f - 40.0f;
            emit packTempUpdated(packTemp);

            emit chargingStateUpdated(data[3] != 0);

            uint16_t volt_raw = static_cast<uint16_t>(data[4] | (data[5] << 8));
            emit packVoltageUpdated(volt_raw * 0.1f);
        }
        break;
    }
    // TODO(team): add a case here for Member 2's fault/DTC frame once its
    // CAN ID and byte layout are finalized, and emit faultStatusUpdated()
    // from it instead of relying on simulateFault().
    default:
        break;
    }
}
