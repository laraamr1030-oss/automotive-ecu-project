#pragma once
// ---------------------------------------------------------------------------
// live_data.h -- the latest value of every signal in docs/signal_dictionary.md
// as this ECU knows it. EngineECU writes it (own sensors + frames received
// from the other ECUs); UDSHandler only reads it to answer ReadDataByIdentifier.
// ---------------------------------------------------------------------------
#include <cstdint>

struct LiveData {
    // Engine ECU, CAN 0x0C0 (own signals)
    float   rpm             = 0.0f;
    float   coolant_temp    = 10.0f;
    float   throttle        = 0.0f;
    uint8_t running         = 0;
    float   battery_voltage = 14.2f;   // 12 V system voltage

    // Engine ECU fault status, CAN 0x0C1 (own signals)
    uint8_t fault_flags = 0;           // bit0 overheat, bit1 misfire, bit2 12V high,
                                       // bit3 pack overtemp, bit4 pack voltage low
    uint8_t dtc_count   = 0;           // stored DTCs (pending + confirmed)
    uint8_t cel_on      = 0;           // 1 = warning lamp requested

    // Transmission ECU, CAN 0x0D0 (received)
    float   vehicle_speed = 0.0f;      // km/h
    uint8_t gear          = 0;

    // Body Control Module, CAN 0x320 (received, kept as raw bytes)
    uint8_t bcm_doors    = 0;          // byte0: bit0..3 doors FL FR RL RR, bit5 hazard
    uint8_t bcm_lights   = 0;          // byte1: bit0 ignition, bit1 left, bit2 right
    uint8_t bcm_batt_raw = 0;          // byte2: factor 0.1 V

    // BMS ECU, CAN 0x350 (received)
    float   soc          = 0.0f;       // %
    float   pack_temp    = 0.0f;       // degC
    uint8_t charging     = 0;
    float   pack_voltage = 0.0f;       // V
};
