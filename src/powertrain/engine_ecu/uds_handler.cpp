#include "uds_handler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <thread>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/select.h>
#include <unistd.h>

namespace {
// Float -> unsigned integer with rounding and clamping (a plain cast of a
// negative or too-large float is undefined behaviour).
uint16_t toU16(float v) { return static_cast<uint16_t>(std::lround(std::clamp(v, 0.0f, 65535.0f))); }
uint8_t  toU8 (float v) { return static_cast<uint8_t >(std::lround(std::clamp(v, 0.0f, 255.0f))); }
void pushU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));   // big-endian on UDS
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}
}  // namespace

UDSHandler::UDSHandler(const LiveData& live, FaultManager& dtc_manager)
    : live_(live), dtc_manager_(dtc_manager) {}

void UDSHandler::handleRequest(const can_frame& req, int can_sock) {
    if (req.can_dlc < 2) return;

    // Byte 0 is the ISO-TP PCI. Only Single Frames (high nibble 0) are
    // requests; a stray Flow Control (0x3X) must not be parsed as a service.
    if ((req.data[0] & 0xF0) != 0x00) return;

    uint8_t sid = req.data[1];
    std::cout << "[UDS] Received SID 0x" << std::hex << static_cast<int>(sid) << std::dec << "\n";

    switch (sid) {
        case 0x22: handleReadDataByID(req, can_sock); break;
        case 0x19: handleReadDTCs(req, can_sock);     break;
        case 0x14: handleClearDTCs(req, can_sock);    break;
        case 0x11: handleECUReset(req, can_sock);     break;
        default:   sendNegativeResponse(sid, 0x11, can_sock); break;   // serviceNotSupported
    }
}

bool UDSHandler::appendDidData(uint16_t did, const LiveData& d, std::vector<uint8_t>& out) const {
    switch (did) {
        case DID_ENGINE_RPM:      pushU16(out, toU16(d.rpm / 0.25f));              return true;
        case DID_COOLANT_TEMP:    out.push_back(toU8(d.coolant_temp + 40.0f));     return true;
        case DID_BATTERY_VOLTAGE: pushU16(out, toU16(d.battery_voltage / 0.01f));  return true;
        case DID_THROTTLE:        out.push_back(toU8(d.throttle / 0.4f));          return true;
        case DID_ENGINE_RUNNING:  out.push_back(d.running);                        return true;
        case DID_FAULT_FLAGS:     out.push_back(d.fault_flags);                    return true;
        case DID_DTC_COUNT:       out.push_back(d.dtc_count);                      return true;
        case DID_VEHICLE_SPEED:   pushU16(out, toU16(d.vehicle_speed / 0.01f));    return true;
        case DID_CURRENT_GEAR:    out.push_back(d.gear);                           return true;
        case DID_DOOR_STATUS:     out.push_back(d.bcm_doors);                      return true;
        case DID_LIGHT_STATUS:    out.push_back(d.bcm_lights);                     return true;
        case DID_BCM_BATT_VOLT:   out.push_back(d.bcm_batt_raw);                   return true;
        case DID_BMS_SOC:         pushU16(out, toU16(d.soc / 0.4f));               return true;
        case DID_BMS_PACK_TEMP:   out.push_back(toU8((d.pack_temp + 40.0f) / 0.5f)); return true;
        case DID_BMS_CHARGING:    out.push_back(d.charging);                       return true;
        case DID_BMS_PACK_VOLT:   pushU16(out, toU16(d.pack_voltage / 0.1f));      return true;
        default: return false;
    }
}

void UDSHandler::handleReadDataByID(const can_frame& req, int sock) {
    // Request: 03 22 <didHigh> <didLow>
    if (req.can_dlc < 4) {
        sendNegativeResponse(0x22, 0x13, sock);   // incorrectMessageLengthOrInvalidFormat
        return;
    }
    uint16_t did = static_cast<uint16_t>((req.data[2] << 8) | req.data[3]);

    std::vector<uint8_t> payload = {0x62, req.data[2], req.data[3]};   // positive response SID + echoed DID
    if (!appendDidData(did, live_, payload)) {
        sendNegativeResponse(0x22, 0x31, sock);   // requestOutOfRange (unknown DID)
        return;
    }
    isoTpSend(payload, sock);
    std::cout << "[UDS] ReadDataByIdentifier 0x" << std::hex << did << std::dec
              << " -> " << payload.size() - 3 << " data byte(s)\n";
}

void UDSHandler::handleReadDTCs(const can_frame& req, int sock) {
    // Request: 03 19 02 <statusMask>     (reportDTCByStatusMask)
    //     or:  04 19 04 <high> <low> ... (freeze frame, handled separately)
    if (req.can_dlc < 3) {
        sendNegativeResponse(0x19, 0x13, sock);
        return;
    }
    uint8_t sub_function = req.data[2];
    if (sub_function == 0x04) { handleReadFreezeFrame(req, sock); return; }
    if (sub_function != 0x02) {
        sendNegativeResponse(0x19, 0x12, sock);   // subFunctionNotSupported
        return;
    }
    if (req.can_dlc < 4) {
        sendNegativeResponse(0x19, 0x13, sock);
        return;
    }
    uint8_t mask = req.data[3];

    std::vector<uint8_t> payload = {0x59, 0x02, 0xFF};   // response SID, sub-function, availabilityMask
    int reported = 0;
    for (const auto& d : dtc_manager_.getActiveDTCs()) {
        if ((d.status & mask) == 0) continue;            // ISO: report only DTCs matching the mask
        payload.push_back(d.high);
        payload.push_back(d.low);
        payload.push_back(d.status);
        ++reported;
    }
    std::cout << "[UDS] ReadDTCInformation mask 0x" << std::hex << static_cast<int>(mask) << std::dec
              << ": " << reported << " DTC(s), " << payload.size() << " bytes via ISO-TP\n";
    isoTpSend(payload, sock);
}

void UDSHandler::handleReadFreezeFrame(const can_frame& req, int sock) {
    // Request: 04 19 04 <high> <low> <recordNumber>
    if (req.can_dlc < 5) {
        sendNegativeResponse(0x19, 0x13, sock);
        return;
    }
    uint8_t high = req.data[3];
    uint8_t low  = req.data[4];

    const DTC* found = nullptr;
    std::vector<DTC> stored = dtc_manager_.getActiveDTCs();   // local copy keeps `found` valid
    for (const auto& d : stored) {
        if (d.high == high && d.low == low) { found = &d; break; }
    }
    if (found == nullptr || !found->freeze_frame.captured) {
        sendNegativeResponse(0x19, 0x31, sock);   // requestOutOfRange: no such DTC / no snapshot
        return;
    }

    // Turn the stored snapshot back into a LiveData so the SAME encoder as
    // ReadDataByIdentifier produces the bytes -- the two can never disagree.
    const FreezeFrame& ff = found->freeze_frame;
    LiveData snap;
    snap.rpm = ff.rpm;               snap.coolant_temp = ff.coolant_temp;
    snap.battery_voltage = ff.battery_voltage;
    snap.soc = ff.soc;               snap.pack_temp = ff.pack_temp;
    snap.pack_voltage = ff.pack_voltage;

    static const uint16_t kSnapshotDids[] = {
        DID_ENGINE_RPM, DID_COOLANT_TEMP, DID_BATTERY_VOLTAGE,
        DID_BMS_SOC, DID_BMS_PACK_TEMP, DID_BMS_PACK_VOLT
    };
    const uint8_t n_ids = static_cast<uint8_t>(sizeof(kSnapshotDids) / sizeof(kSnapshotDids[0]));

    std::vector<uint8_t> payload = {0x59, 0x04, high, low, found->status,
                                    0x01,      // DTCSnapshotRecordNumber (one per DTC)
                                    n_ids};    // numberOfIdentifiers
    for (uint16_t did : kSnapshotDids) {
        payload.push_back(static_cast<uint8_t>((did >> 8) & 0xFF));
        payload.push_back(static_cast<uint8_t>(did & 0xFF));
        appendDidData(did, snap, payload);
    }
    std::cout << "[UDS] Freeze frame for DTC 0x" << std::hex << static_cast<int>(high)
              << static_cast<int>(low) << std::dec << " (" << payload.size() << " bytes)\n";
    isoTpSend(payload, sock);
}

void UDSHandler::handleClearDTCs(const can_frame& /*req*/, int sock) {
    // Request: 04 14 FF FF FF (groupOfDTC = all). clearAll() removes the
    // records entirely -- zeroing only testFailed would leave them "active".
    dtc_manager_.clearAll();
    isoTpSend({0x54}, sock);   // positive response to 0x14
    std::cout << "[UDS] DTCs cleared.\n";
}

void UDSHandler::handleECUReset(const can_frame& req, int sock) {
    // Request: 02 11 01 (hardReset)
    if (req.can_dlc < 3 || req.data[2] != 0x01) {
        sendNegativeResponse(0x11, 0x12, sock);   // subFunctionNotSupported
        return;
    }
    isoTpSend({0x51, 0x01}, sock);
    reset_requested_ = true;
    std::cout << "[UDS] ECU hard reset requested.\n";
}

void UDSHandler::sendNegativeResponse(uint8_t sid, uint8_t nrc, int sock) {
    uint8_t resp[8] = {0x03, 0x7F, sid, nrc, 0, 0, 0, 0};
    sendFrame(resp, 8, sock);
    std::cout << "[UDS] NRC 0x" << std::hex << (int)nrc
              << " for SID 0x" << (int)sid << std::dec << "\n";
}

void UDSHandler::sendFrame(const uint8_t* data, uint8_t len, int sock) {
    struct can_frame frame{};
    frame.can_id  = UDS_RESPONSE_ID;
    frame.can_dlc = 8;
    std::memcpy(frame.data, data, std::min<uint8_t>(len, 8));
    if (write(sock, &frame, sizeof(frame)) < 0) {
        perror("[UDS] write");
    }
}

bool UDSHandler::waitForFrame(int sock, uint32_t expect_id, int timeout_ms, can_frame& out) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (true) {
        auto remaining = deadline - std::chrono::steady_clock::now();
        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();
        if (remaining_ms <= 0) return false;

        fd_set set;
        FD_ZERO(&set);
        FD_SET(sock, &set);
        struct timeval tv{};
        tv.tv_sec  = remaining_ms / 1000;
        tv.tv_usec = (remaining_ms % 1000) * 1000;

        int rv = select(sock + 1, &set, nullptr, nullptr, &tv);
        if (rv <= 0) return false; // timeout or error

        can_frame candidate{};
        ssize_t n = read(sock, &candidate, sizeof(candidate));
        if (n <= 0) return false;
        if (candidate.can_id == expect_id) {
            out = candidate;
            return true;
        }
        // Not the frame we're waiting for (e.g. a stray frame) -- keep
        // waiting until the deadline instead of giving up on the first miss.
    }
}

bool UDSHandler::isoTpSend(const std::vector<uint8_t>& payload, int sock) {
    if (payload.size() <= 7) {
        // Single Frame: PCI = 0x0 | length in the low nibble.
        uint8_t frame[8] = {};
        frame[0] = static_cast<uint8_t>(payload.size());
        std::copy(payload.begin(), payload.end(), frame + 1);
        sendFrame(frame, 8, sock);
        return true;
    }

    // Multi-frame: First Frame carries the 12-bit total length and the
    // first 6 payload bytes (Session 12 Section 3.2). A multi-DTC
    // ReadDTCInformation response is the case in this demo that actually
    // exercises this path.
    uint16_t total_len = static_cast<uint16_t>(payload.size());
    uint8_t ff[8] = {};
    ff[0] = 0x10 | static_cast<uint8_t>((total_len >> 8) & 0x0F);
    ff[1] = static_cast<uint8_t>(total_len & 0xFF);
    std::copy(payload.begin(), payload.begin() + 6, ff + 2);
    sendFrame(ff, 8, sock);
    std::cout << "[ISO-TP] Sent First Frame (total length " << total_len << ")\n";

    size_t sent = 6;
    uint8_t seq = 1;
    uint8_t frames_since_fc = 0;
    uint8_t block_size = 0;
    uint8_t stmin_ms = 0;

    // First Flow Control must arrive before any Consecutive Frame is sent.
    can_frame fc{};
    if (!waitForFrame(sock, UDS_REQUEST_ID, 2000, fc) || (fc.data[0] & 0xF0) != 0x30) {
        std::cerr << "[ISO-TP] Timed out (or got a non-FC frame) waiting for Flow Control.\n";
        return false;
    }
    uint8_t flow_status = fc.data[0] & 0x0F;
    if (flow_status == 2) {
        std::cerr << "[ISO-TP] Flow Control: Overflow/abort -- receiver rejected the transfer.\n";
        return false;
    }
    block_size = fc.data[1];
    stmin_ms   = fc.data[2];
    std::cout << "[ISO-TP] Flow Control received: CTS, BlockSize=" << (int)block_size
              << ", STmin=" << (int)stmin_ms << "ms\n";

    while (sent < payload.size()) {
        uint8_t cf[8] = {};
        cf[0] = 0x20 | (seq & 0x0F);
        size_t chunk = std::min<size_t>(7, payload.size() - sent);
        std::copy(payload.begin() + sent, payload.begin() + sent + chunk, cf + 1);
        sendFrame(cf, 8, sock);
        sent += chunk;
        seq = (seq + 1) % 16; // wraps 0-15, restarting at 0 after 15

        if (stmin_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(stmin_ms));
        }

        if (sent >= payload.size()) break;

        // BlockSize 0 means "send them all, no further FC needed."
        if (block_size != 0 && ++frames_since_fc >= block_size) {
            frames_since_fc = 0;
            if (!waitForFrame(sock, UDS_REQUEST_ID, 2000, fc) || (fc.data[0] & 0xF0) != 0x30) {
                std::cerr << "[ISO-TP] Timed out waiting for next Flow Control block.\n";
                return false;
            }
            block_size = fc.data[1];
            stmin_ms   = fc.data[2];
        }
    }

    std::cout << "[ISO-TP] Sent " << payload.size() << " bytes total.\n";
    return true;
}
