// Offline self-test for FaultManager + UDSHandler -- needs NO vcan0 and NO
// other ECU. A Unix socketpair stands in for the CAN bus: the handler writes
// its responses to one end, this test reads them from the other end.
//
// Build & run (from the repo root):
//   g++ -std=c++17 -Wall -Isrc/powertrain/engine_ecu tests/test_diagnostics_offline.cpp src/powertrain/engine_ecu/uds_handler.cpp -o build/test_diag -pthread
//   ./build/test_diag
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include "uds_handler.h"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (cond) std::printf("  [PASS] %s\n", msg); \
                              else { std::printf("  [FAIL] %s\n", msg); ++g_fail; } } while (0)

static can_frame makeFrame(uint32_t id, std::vector<uint8_t> d) {
    can_frame f{}; f.can_id = id; f.can_dlc = 8;
    for (size_t i = 0; i < d.size() && i < 8; ++i) f.data[i] = d[i];
    return f;
}

// Sends one request to the handler and returns the fully reassembled response payload.
static std::vector<uint8_t> transact(UDSHandler& h, int ecu_fd, int tester_fd, std::vector<uint8_t> req, int dlc = 8) {
    can_frame rf = makeFrame(UDS_REQUEST_ID, req);
    rf.can_dlc = static_cast<uint8_t>(dlc);   // a real short request has a short DLC
    std::thread ecu([&] { h.handleRequest(rf, ecu_fd); });

    std::vector<uint8_t> out;
    can_frame r{};
    if (read(tester_fd, &r, sizeof r) == sizeof r) {
        uint8_t type = r.data[0] >> 4;
        if (type == 0) {                                   // Single Frame
            out.assign(r.data + 1, r.data + 1 + (r.data[0] & 0x0F));
        } else if (type == 1) {                            // First Frame -> answer with Flow Control
            size_t total = ((r.data[0] & 0x0F) << 8) | r.data[1];
            out.assign(r.data + 2, r.data + 8);
            can_frame fc = makeFrame(UDS_REQUEST_ID, {0x30, 0x00, 0x00});
            write(tester_fd, &fc, sizeof fc);
            while (out.size() < total) {                   // Consecutive Frames
                can_frame cf{};
                if (read(tester_fd, &cf, sizeof cf) != sizeof cf) break;
                out.insert(out.end(), cf.data + 1, cf.data + 8);
            }
            out.resize(total);
        }
    }
    ecu.join();
    return out;
}

int main() {
    int sv[2];
    socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);

    LiveData live;
    live.rpm = 2200.0f; live.coolant_temp = 90.0f; live.battery_voltage = 14.2f;
    live.throttle = 33.0f; live.running = 1; live.vehicle_speed = 62.5f; live.gear = 4;
    live.soc = 84.0f; live.pack_temp = 30.0f; live.pack_voltage = 350.0f; live.charging = 0;
    FaultManager fm;
    UDSHandler uds(live, fm);

    std::printf("== FaultManager lifecycle ==\n");
    FreezeFrame snap; snap.rpm = 2200; snap.coolant_temp = 107; snap.pack_temp = 58;
    fm.evaluate(0x01, 0x17, true, snap);
    CHECK(fm.getActiveDTCs().at(0).status == 0x05, "1st failing cycle -> 0x05 (testFailed+pending)");
    CHECK(!fm.hasWarning(), "CEL still OFF while only pending");
    snap.coolant_temp = 999;   // must NOT overwrite the stored freeze frame
    fm.evaluate(0x01, 0x17, true, snap);
    CHECK(fm.getActiveDTCs().at(0).status == 0x8D, "2nd failing cycle -> 0x8D (confirmed+warning)");
    CHECK(fm.hasWarning(), "CEL ON after confirmation");
    CHECK(fm.getActiveDTCs().at(0).freeze_frame.coolant_temp == 107.0f, "freeze frame captured ONCE (first values kept)");
    fm.evaluate(0x01, 0x17, false, snap);
    CHECK(fm.getActiveDTCs().at(0).status == 0x88, "condition gone -> 0x88, DTC persists");
    CHECK(fm.hasWarning(), "CEL stays ON until cleared");
    fm.evaluate(0x03, 0x00, true, snap);
    fm.evaluate(0x03, 0x00, false, snap);
    CHECK(fm.getActiveDTCs().size() == 1, "single-cycle glitch (never confirmed) is discarded");

    std::printf("== UDS 0x22 ReadDataByIdentifier ==\n");
    auto r = transact(uds, sv[0], sv[1], {0x03, 0x22, 0x01, 0x01});
    CHECK(r == std::vector<uint8_t>({0x62, 0x01, 0x01, 0x22, 0x60}), "RPM 2200 -> raw 0x2260 (8800 * 0.25)");
    r = transact(uds, sv[0], sv[1], {0x03, 0x22, 0x02, 0x01});
    CHECK(r == std::vector<uint8_t>({0x62, 0x02, 0x01, 0x18, 0x6A}), "Speed 62.5 km/h -> raw 0x186A (6250 * 0.01)");
    r = transact(uds, sv[0], sv[1], {0x03, 0x22, 0x04, 0x01});
    CHECK(r == std::vector<uint8_t>({0x62, 0x04, 0x01, 0x00, 0xD2}), "SoC 84 % -> raw 210 (0x00D2) (84 / 0.4)");
    r = transact(uds, sv[0], sv[1], {0x03, 0x22, 0x04, 0x02});
    CHECK(r == std::vector<uint8_t>({0x62, 0x04, 0x02, 0x8C}), "Pack temp 30 C -> raw 140 (0x8C) ((30+40)/0.5)");
    r = transact(uds, sv[0], sv[1], {0x03, 0x22, 0x99, 0x99});
    CHECK(r == std::vector<uint8_t>({0x7F, 0x22, 0x31}), "unknown DID -> NRC 0x31 requestOutOfRange");
    r = transact(uds, sv[0], sv[1], {0x02, 0x22, 0x01}, 3);
    CHECK(r == std::vector<uint8_t>({0x7F, 0x22, 0x13}), "short request -> NRC 0x13");
    r = transact(uds, sv[0], sv[1], {0x02, 0x27, 0x01});
    CHECK(r == std::vector<uint8_t>({0x7F, 0x27, 0x11}), "unsupported SID -> NRC 0x11");

    std::printf("== UDS 0x19 ReadDTCInformation ==\n");
    fm.evaluate(0x0A, 0x7E, true, snap);   // add a 2nd DTC (pending) so the reply needs ISO-TP multi-frame
    r = transact(uds, sv[0], sv[1], {0x03, 0x19, 0x02, 0xFF});
    CHECK(r == std::vector<uint8_t>({0x59, 0x02, 0xFF, 0x01, 0x17, 0x88, 0x0A, 0x7E, 0x05}),
          "mask 0xFF lists both DTCs (P0117=0x88, P0A7E=0x05) via multi-frame ISO-TP");
    r = transact(uds, sv[0], sv[1], {0x03, 0x19, 0x02, 0x08});
    CHECK(r == std::vector<uint8_t>({0x59, 0x02, 0xFF, 0x01, 0x17, 0x88}), "mask 0x08 lists only the confirmed DTC");
    r = transact(uds, sv[0], sv[1], {0x04, 0x19, 0x04, 0x01, 0x17, 0x01});
    CHECK(r.size() == 29 && r[0] == 0x59 && r[1] == 0x04 && r[6] == 0x06, "freeze frame: 29 bytes, 6 identifiers");
    r = transact(uds, sv[0], sv[1], {0x04, 0x19, 0x04, 0x09, 0x99, 0x01});
    CHECK(r == std::vector<uint8_t>({0x7F, 0x19, 0x31}), "freeze frame of unknown DTC -> NRC 0x31");
    r = transact(uds, sv[0], sv[1], {0x03, 0x19, 0x07, 0xFF});
    CHECK(r == std::vector<uint8_t>({0x7F, 0x19, 0x12}), "unsupported sub-function -> NRC 0x12");

    std::printf("== UDS 0x14 / 0x11 ==\n");
    r = transact(uds, sv[0], sv[1], {0x04, 0x14, 0xFF, 0xFF, 0xFF});
    CHECK(r == std::vector<uint8_t>({0x54}) && fm.getActiveDTCs().empty() && !fm.hasWarning(),
          "ClearDiagnosticInformation -> 0x54, store empty, CEL off");
    r = transact(uds, sv[0], sv[1], {0x02, 0x11, 0x02});
    CHECK(r == std::vector<uint8_t>({0x7F, 0x11, 0x12}), "ECUReset bad sub-function -> NRC 0x12");
    r = transact(uds, sv[0], sv[1], {0x02, 0x11, 0x01});
    CHECK(r == std::vector<uint8_t>({0x51, 0x01}) && uds.consumeResetRequest() && !uds.consumeResetRequest(),
          "ECUReset -> 51 01, flag consumed exactly once");

    std::printf("\n%s (%d failure(s))\n", g_fail ? "SOME TESTS FAILED" : "ALL TESTS PASSED", g_fail);
    return g_fail ? 1 : 0;
}
