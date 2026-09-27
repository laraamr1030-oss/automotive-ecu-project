#pragma once
#include <cstdint>
#include <vector>
#include <linux/can.h>

#include "fault_manager.h"

constexpr uint32_t UDS_REQUEST_ID  = 0x7E0;
constexpr uint32_t UDS_RESPONSE_ID = 0x7E8;

constexpr uint16_t DID_ENGINE_RPM      = 0x0101;
constexpr uint16_t DID_COOLANT_TEMP    = 0x0102;
constexpr uint16_t DID_BATTERY_VOLTAGE = 0x0103; // only surfaced inside freeze-frame snapshots
constexpr uint16_t DID_VEHICLE_SPEED   = 0x0201;

// Session 13's ISO-TP transport (isoTpSend/waitForFrame), reused verbatim
// here, now also carrying SID 0x19 ReadDTCInformation responses -- multiple
// 3-byte DTC records routinely exceed the 7-byte Single Frame limit, which
// is exactly the case isoTpSend() exists to handle.
class UDSHandler {
public:
    // rpm_ref/temp_ref are references to EngineECU's live sensor values;
    // dtc_manager is the *shared* fault manager EngineECU::cyclicTask()
    // writes into every monitoring cycle -- there is only ever one DTC
    // store per ECU, never a second one owned by the UDS handler (that was
    // the Session 13 demo's placeholder; this demo wires it to the real one).
UDSHandler(const float& rpm_ref,const float& temp_ref,const float& vehicle_speed_ref,const float& battery_voltage_ref, FaultManager& dtc_manager);
    // Call when a CAN frame with ID == UDS_REQUEST_ID is received
    void handleRequest(const can_frame& req, int can_sock);

    // Consumes the pending reset flag: returns whether a reset was
    // requested since the last call, and clears it. Must be consumed (not
    // just read) -- otherwise every cyclic tick re-triggers the reset.
    bool consumeResetRequest() {
        bool r = reset_requested_;
        reset_requested_ = false;
        return r;
    }

private:
    void handleReadDataByID(const can_frame& req, int sock);
    void handleReadDTCs(const can_frame& req, int sock);
    // SID 0x19 sub-function 0x04 (reportDTCSnapshotRecordByDTCNumber,
    // content.md's freeze-frame objective -- see fault_manager.h's
    // FreezeFrame comment for why this demo's wire format is its own
    // design rather than something content.md specifies).
    void handleReadFreezeFrame(const can_frame& req, int sock);
    void handleClearDTCs(const can_frame& req, int sock);
    void handleECUReset(const can_frame& req, int sock);
    void sendNegativeResponse(uint8_t sid, uint8_t nrc, int sock);
    void sendFrame(const uint8_t* data, uint8_t len, int sock);

    // --- ISO-TP transport (Session 12, Section 3 / Session 13 demo) ---
    // Sends `payload` as a Single Frame if it fits in 7 bytes, otherwise as
    // a correct First Frame + Flow-Control-gated Consecutive Frame sequence.
    bool isoTpSend(const std::vector<uint8_t>& payload, int sock);
    // Blocks (up to timeout_ms) for the next frame whose can_id == expect_id.
    // Used only to wait for the tester's Flow Control frame after a First
    // Frame -- everything else this responder does stays non-blocking.
    bool waitForFrame(int sock, uint32_t expect_id, int timeout_ms, can_frame& out);

    const float& rpm_;
    const float& coolant_temp_;
    const float& vehicle_speed_;
    const float& battery_voltage_;
    FaultManager& dtc_manager_;
    bool reset_requested_ = false;
};
