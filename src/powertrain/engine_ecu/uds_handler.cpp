#include "uds_handler.h"

#include <cstring>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/select.h>
#include <unistd.h>

UDSHandler::UDSHandler(const float& rpm_ref,const float& temp_ref,const float& vehicle_speed_ref,const float& battery_voltage_ref,FaultManager& dtc_manager): rpm_(rpm_ref),coolant_temp_(temp_ref),vehicle_speed_(vehicle_speed_ref),battery_voltage_(battery_voltage_ref),dtc_manager_(dtc_manager) {}

void UDSHandler::handleRequest(const can_frame& req, int can_sock) {
    if (req.can_dlc < 2) return;

    uint8_t sid = req.data[1];
    std::cout << "[UDS] Received SID 0x" << std::hex << (int)sid << std::dec << "\n";

    switch (sid) {
        case 0x22: handleReadDataByID(req, can_sock); break;
        case 0x19: handleReadDTCs(req, can_sock);      break;
        case 0x14: handleClearDTCs(req, can_sock);      break;
        case 0x11: handleECUReset(req, can_sock);        break;
        default:   sendNegativeResponse(sid, 0x11, can_sock); break;
    }
}

void UDSHandler::handleReadDataByID(const can_frame& req, int sock) {
    if (req.can_dlc < 4) {
        sendNegativeResponse(0x22, 0x13, sock);
        return;
    }

    uint16_t did = (static_cast<uint16_t>(req.data[2]) << 8) | req.data[3];

    if (did == DID_ENGINE_RPM) {
        uint16_t raw = static_cast<uint16_t>(rpm_ / 0.25f);
        std::vector<uint8_t> payload = {
            0x62, 0x01, 0x01,
            static_cast<uint8_t>((raw >> 8) & 0xFF),
            static_cast<uint8_t>(raw & 0xFF)
        };
        isoTpSend(payload, sock);
        std::cout << "[UDS] RPM=" << static_cast<int>(rpm_) << " raw=0x" << std::hex << raw << std::dec << "\n";
    }
    else if (did == DID_COOLANT_TEMP) {
        // factor=1.0, offset=-40 -- must match engine_ecu.cpp's
        // packEngineStatus() and CANWorker.cpp's/uds_tester.py's decode.
        uint8_t raw = static_cast<uint8_t>(coolant_temp_ + 40.0f);
        std::vector<uint8_t> payload = {0x62, 0x01, 0x02, raw};
        isoTpSend(payload, sock);
        std::cout << "[UDS] Temp=" << coolant_temp_ << "\xC2\xB0" << "C raw=0x" << std::hex << (int)raw << std::dec << "\n";
    }
        else if (did == DID_VEHICLE_SPEED) {
    // Vehicle Speed:
    // CAN signal: 16 bits, factor = 0.01 km/h, offset = 0
    std::cout << "[DEBUG] vehicle_speed_="
          << vehicle_speed_ << " km/h\n";
    uint16_t raw = static_cast<uint16_t>(vehicle_speed_ / 0.01f);

    std::vector<uint8_t> payload = {
        0x62, 0x02, 0x01,
        static_cast<uint8_t>((raw >> 8) & 0xFF),
        static_cast<uint8_t>(raw & 0xFF)
    };

    isoTpSend(payload, sock);

    std::cout << "[UDS] Vehicle Speed="
              << vehicle_speed_
              << " km/h raw=0x"
              << std::hex << raw << std::dec << "\n";
        }
         else if (did == DID_BATTERY_VOLTAGE) {
        // Battery Voltage:
        // factor = 0.01 V, offset = 0
        uint16_t raw =
            static_cast<uint16_t>(battery_voltage_ / 0.01f);

        std::vector<uint8_t> payload = {
            0x62, 0x01, 0x03,
            static_cast<uint8_t>((raw >> 8) & 0xFF),
            static_cast<uint8_t>(raw & 0xFF)
        };

        isoTpSend(payload, sock);

        std::cout << "[UDS] Battery Voltage="
                  << battery_voltage_
                  << " V raw=0x"
                  << std::hex << raw << std::dec << "\n";
        }
    else {
        sendNegativeResponse(0x22, 0x31, sock);
    }
}

void UDSHandler::handleReadDTCs(const can_frame& req, int sock) {
    // Request: 03 19 02 FF 00 00 00 00
    //          SID=0x19, subFunction=0x02 (reportDTCByStatusMask), mask=0xFF
    if (req.can_dlc < 3) {
        sendNegativeResponse(0x19, 0x13, sock);
        return;
    }
    uint8_t sub_function = req.data[2];
    if (sub_function == 0x04) {
        handleReadFreezeFrame(req, sock);
        return;
    }
    if (sub_function != 0x02) {
        sendNegativeResponse(0x19, 0x12, sock); // subFunctionNotSupported
        return;
    }

    // This demo doesn't filter by the requested status mask -- it reports
    // every DTC the FaultManager currently considers active (status != 0),
    // which is the same set getActiveDTCs() (and Pitfall 1's clearAll()
    // fix) already govern.
    std::vector<DTC> active = dtc_manager_.getActiveDTCs();

    std::vector<uint8_t> payload;
    payload.reserve(3 + active.size() * 3);
    payload.push_back(0x59);
    payload.push_back(0x02);
    payload.push_back(0xFF); // DTCStatusAvailabilityMask

    for (const auto& d : active) {
        payload.push_back(d.high);
        payload.push_back(d.low);
        payload.push_back(d.status);
    }

    std::cout << "[UDS] ReadDTCInformation: " << active.size()
              << " active DTC(s), sending " << payload.size() << " bytes via ISO-TP\n";
    isoTpSend(payload, sock);
}

void UDSHandler::handleReadFreezeFrame(const can_frame& req, int sock) {
    // Request: 04 19 04 <high> <low> 00 00 00
    //          SID=0x19, subFunction=0x04, DTC identified by the same
    //          2-byte (high, low) pair used everywhere else in this demo
    //          (see fault_manager.h's DTC comment on the 2-byte-vs-3-byte
    //          departure from the real spec). The trailing snapshot-record
    //          number byte (real ISO 14229-1 sends one) is accepted but
    //          ignored -- this demo only ever keeps one snapshot per DTC.
    if (req.can_dlc < 5) {
        sendNegativeResponse(0x19, 0x13, sock);
        return;
    }
    uint8_t high = req.data[3];
    uint8_t low  = req.data[4];

    std::vector<DTC> active = dtc_manager_.getActiveDTCs();
    const DTC* found = nullptr;
    for (const auto& d : active) {
        if (d.high == high && d.low == low) { found = &d; break; }
    }
    if (found == nullptr || !found->freeze_frame.captured) {
        sendNegativeResponse(0x19, 0x31, sock); // requestOutOfRange -- no such DTC / no snapshot
        return;
    }

    const FreezeFrame& ff = found->freeze_frame;
    uint16_t rpm_raw  = static_cast<uint16_t>(ff.rpm / 0.25f);
    // Same factor/offset pairs as handleReadDataByID -- must stay in sync.
    uint8_t  temp_raw = static_cast<uint8_t>(ff.coolant_temp + 40.0f);
    uint8_t  volt_raw = static_cast<uint8_t>(ff.battery_voltage / 0.1f);

    std::vector<uint8_t> payload = {
        0x59, 0x04, high, low, found->status,
        0x01,       // DTCSnapshotRecordNumber (only ever one, per DTC)
        0x03,       // numberOfIdentifiers
        static_cast<uint8_t>((DID_ENGINE_RPM >> 8) & 0xFF), static_cast<uint8_t>(DID_ENGINE_RPM & 0xFF),
        static_cast<uint8_t>((rpm_raw >> 8) & 0xFF), static_cast<uint8_t>(rpm_raw & 0xFF),
        static_cast<uint8_t>((DID_COOLANT_TEMP >> 8) & 0xFF), static_cast<uint8_t>(DID_COOLANT_TEMP & 0xFF),
        temp_raw,
        static_cast<uint8_t>((DID_BATTERY_VOLTAGE >> 8) & 0xFF), static_cast<uint8_t>(DID_BATTERY_VOLTAGE & 0xFF),
        volt_raw
    };

    std::cout << "[UDS] ReadDTCSnapshot for 0x" << std::hex << (int)high << (int)low << std::dec
              << ": RPM=" << ff.rpm << " Temp=" << ff.coolant_temp
              << " Volt=" << ff.battery_voltage << "\n";
    isoTpSend(payload, sock);
}

void UDSHandler::handleClearDTCs(const can_frame& /*req*/, int sock) {
    // Pitfall 1 (content.md Section 2.3): a "clear" that only zeroes
    // testFailed leaves pendingDTC/confirmedDTC set, so getActiveDTCs()
    // still reports the DTC. clearAll() removes the records entirely.
    dtc_manager_.clearAll();
    std::vector<uint8_t> payload = {0x54};
    isoTpSend(payload, sock);
    std::cout << "[UDS] DTCs cleared.\n";
}

void UDSHandler::handleECUReset(const can_frame& req, int sock) {
    if (req.can_dlc < 3 || req.data[2] != 0x01) {
        sendNegativeResponse(0x11, 0x12, sock); // subFunctionNotSupported
        return;
    }
    std::vector<uint8_t> payload = {0x51, 0x01};
    isoTpSend(payload, sock);
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
