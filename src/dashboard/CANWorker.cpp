#include "CANWorker.h"
#include <QDebug>
#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

CANWorker::CANWorker(QObject *parent) : QObject(parent) {}

CANWorker::~CANWorker() {
    stop();
}

void CANWorker::process() {
    run();
}

void CANWorker::stop() {
    stop_ = true;   // run() polls with a 100 ms timeout, sees this, closes the socket itself
}

bool CANWorker::openSocket() {
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) return false;

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        close(fd);
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return false;
    }

    can_socket_ = fd;
    return true;
}

void CANWorker::run() {
    if (!openSocket()) {
        qWarning() << "Failed to open vcan0 socket (is vcan0 up?)";
        return;
    }

    struct pollfd pfd;
    pfd.fd = can_socket_;
    pfd.events = POLLIN;

    while (!stop_) {
        // Wait for a frame, but wake up every 100 ms so stop() is noticed.
        int rc = poll(&pfd, 1, 100);
        if (rc < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (rc == 0) continue;   // timeout, no frame

        struct can_frame frame;
        ssize_t nbytes = read(can_socket_, &frame, sizeof(frame));
        if (nbytes < 0) break;
        if (nbytes == static_cast<ssize_t>(sizeof(frame))) {
            decodeAndEmit(frame.can_id, frame.data, frame.can_dlc);
        }
    }

    int fd = can_socket_.exchange(-1);
    if (fd >= 0) close(fd);
}

bool CANWorker::sendClearDtcRequest() {
    int fd = can_socket_;
    if (fd < 0) return false;

    struct can_frame req;
    std::memset(&req, 0, sizeof(req));
    req.can_id  = 0x7E0;                    // UDS request ID (tester -> ECU)
    req.can_dlc = 8;
    req.data[0] = 0x04;                     // ISO-TP Single Frame, 4 payload bytes
    req.data[1] = 0x14;                     // SID ClearDiagnosticInformation
    req.data[2] = 0xFF;                     // groupOfDTC = 0xFFFFFF (all DTCs)
    req.data[3] = 0xFF;
    req.data[4] = 0xFF;
    return write(fd, &req, sizeof(req)) == static_cast<ssize_t>(sizeof(req));
}

void CANWorker::decodeAndEmit(canid_t can_id, const uint8_t *data, uint8_t dlc) {
    if (!data) return;

    uint32_t id = can_id & CAN_SFF_MASK;

    switch (id) {
    case 0x0C0: { // Engine ECU -- see docs/signal_dictionary.md
        if (dlc >= 8) {
            uint16_t rpm_raw = static_cast<uint16_t>(data[0] | (data[1] << 8));
            emit engineSpeedUpdated(static_cast<int>(rpm_raw * 0.25f));
            emit coolantTempUpdated(static_cast<float>(data[4]) * 1.0f - 40.0f);  // byte 4, NOT byte 2
            emit throttleUpdated(static_cast<float>(data[5]) * 0.4f);
            emit systemVoltageUpdated(static_cast<float>(data[7]) * 0.1f);
        }
        break;
    }
    case 0x0C1: { // Engine ECU fault status -- REAL fault data (replaces simulateFault())
        if (dlc >= 3) {
            emit faultFlagsUpdated(data[0]);
            emit dtcCountUpdated(data[1]);
            emit faultStatusUpdated(data[2] != 0);   // CEL follows the ECU's warning-lamp request
        }
        break;
    }
    case 0x0D0: { // Transmission ECU
        if (dlc >= 3) {
            uint16_t speed_raw = static_cast<uint16_t>(data[0] | (data[1] << 8));
            emit vehicleSpeedUpdated(speed_raw * 0.01f);
            emit currentGearUpdated(static_cast<int>(data[2]));
        }
        break;
    }
    case 0x320: { // Body Control Module -- layout = bcm.cpp packBCMStatus()
        if (dlc >= 2) {
            emit doorStatusUpdated(data[0] & 0x01, data[0] & 0x02, data[0] & 0x04, data[0] & 0x08);
            emit hazardChanged((data[0] & 0x20) != 0);                          // byte0 bit5
            emit turnSignalsChanged((data[1] & 0x02) != 0, (data[1] & 0x04) != 0); // byte1 bit1 left, bit2 right
        }
        break;
    }
    case 0x350: { // BMS ECU -- matches bms_ecu.cpp packBmsStatus()
        if (dlc >= 6) {
            uint16_t soc_raw = static_cast<uint16_t>(data[0] | (data[1] << 8));
            emit socUpdated(soc_raw * 0.4f);
            emit packTempUpdated(static_cast<float>(data[2]) * 0.5f - 40.0f);
            emit chargingStateUpdated(data[3] != 0);
            uint16_t volt_raw = static_cast<uint16_t>(data[4] | (data[5] << 8));
            emit packVoltageUpdated(volt_raw * 0.1f);
        }
        break;
    }
    case 0x7E8: { // UDS response -- we only care about the answer to our own clear request
        if (dlc >= 2 && data[0] == 0x01 && data[1] == 0x54) {
            emit clearDtcResponse(true);                       // 01 54 = positive response to 0x14
        } else if (dlc >= 4 && data[0] == 0x03 && data[1] == 0x7F && data[2] == 0x14) {
            emit clearDtcResponse(false);                      // 03 7F 14 <nrc> = negative response
        }
        break;
    }
    default:
        break;
    }
}
