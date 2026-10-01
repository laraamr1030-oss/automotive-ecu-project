#pragma once
#include <cstdint>
#include <vector>
#include <linux/can.h>

#include "fault_manager.h"
#include "live_data.h"

constexpr uint32_t UDS_REQUEST_ID  = 0x7E0;   // tester -> ECU
constexpr uint32_t UDS_RESPONSE_ID = 0x7E8;   // ECU -> tester

// Data Identifiers: one per signal in docs/signal_dictionary.md.
// The DID's data bytes use the SAME factor/offset as the CAN signal.
constexpr uint16_t DID_ENGINE_RPM      = 0x0101; // u16, 0.25 rpm
constexpr uint16_t DID_COOLANT_TEMP    = 0x0102; // u8,  1.0, offset -40 degC
constexpr uint16_t DID_BATTERY_VOLTAGE = 0x0103; // u16, 0.01 V (12 V system)
constexpr uint16_t DID_THROTTLE        = 0x0104; // u8,  0.4 %
constexpr uint16_t DID_ENGINE_RUNNING  = 0x0105; // u8,  0/1
constexpr uint16_t DID_FAULT_FLAGS     = 0x0106; // u8,  bitfield (see live_data.h)
constexpr uint16_t DID_DTC_COUNT       = 0x0107; // u8
constexpr uint16_t DID_VEHICLE_SPEED   = 0x0201; // u16, 0.01 km/h
constexpr uint16_t DID_CURRENT_GEAR    = 0x0202; // u8
constexpr uint16_t DID_DOOR_STATUS     = 0x0301; // u8 bitfield (BCM byte 0)
constexpr uint16_t DID_LIGHT_STATUS    = 0x0302; // u8 bitfield (BCM byte 1)
constexpr uint16_t DID_BCM_BATT_VOLT   = 0x0303; // u8,  0.1 V
constexpr uint16_t DID_BMS_SOC         = 0x0401; // u16, 0.4 %
constexpr uint16_t DID_BMS_PACK_TEMP   = 0x0402; // u8,  0.5, offset -40 degC
constexpr uint16_t DID_BMS_CHARGING    = 0x0403; // u8,  0/1
constexpr uint16_t DID_BMS_PACK_VOLT   = 0x0404; // u16, 0.1 V

class UDSHandler {
public:
    // live: the ECU's latest signal values (read-only here).
    // dtc_manager: the ONE shared DTC store that EngineECU writes into.
    UDSHandler(const LiveData& live, FaultManager& dtc_manager);

    // Call when a CAN frame with ID == UDS_REQUEST_ID is received.
    void handleRequest(const can_frame& req, int can_sock);

    // Returns true once after an ECUReset request, then clears the flag.
    bool consumeResetRequest() {
        bool r = reset_requested_;
        reset_requested_ = false;
        return r;
    }

private:
    void handleReadDataByID(const can_frame& req, int sock);   // SID 0x22
    void handleReadDTCs(const can_frame& req, int sock);       // SID 0x19 sub 0x02
    void handleReadFreezeFrame(const can_frame& req, int sock);// SID 0x19 sub 0x04
    void handleClearDTCs(const can_frame& req, int sock);      // SID 0x14
    void handleECUReset(const can_frame& req, int sock);       // SID 0x11
    void sendNegativeResponse(uint8_t sid, uint8_t nrc, int sock);
    void sendFrame(const uint8_t* data, uint8_t len, int sock);

    // Appends the data bytes of one DID (big-endian, UDS style) taken from `d`.
    // Returns false if the DID is unknown.
    bool appendDidData(uint16_t did, const LiveData& d, std::vector<uint8_t>& out) const;

    // ISO-TP transport (ISO 15765-2): Single Frame, or First Frame +
    // Flow Control + Consecutive Frames when the payload is > 7 bytes.
    bool isoTpSend(const std::vector<uint8_t>& payload, int sock);
    bool waitForFrame(int sock, uint32_t expect_id, int timeout_ms, can_frame& out);

    const LiveData& live_;
    FaultManager&   dtc_manager_;
    bool reset_requested_ = false;
};
