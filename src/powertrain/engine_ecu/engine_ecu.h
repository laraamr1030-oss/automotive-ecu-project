#pragma once
#include <atomic>
#include <cstdint>

#include "fault_manager.h"
#include "live_data.h"
#include "uds_handler.h"

enum class ECUState { INITIALIZING, RUNNING, FAULT, SHUTTING_DOWN };

// CAN IDs this ECU sends / listens to
constexpr uint32_t ENGINE_STATUS_ID       = 0x0C0;  // sends: rpm, coolant, throttle, running, 12V
constexpr uint32_t FAULT_STATUS_ID        = 0x0C1;  // sends: fault flags, DTC count, CEL request
constexpr uint32_t TRANSMISSION_STATUS_ID = 0x0D0;  // listens: speed + gear
constexpr uint32_t BCM_STATUS_ID          = 0x320;  // listens: doors / lights
constexpr uint32_t BMS_STATUS_ID          = 0x350;  // listens: SoC, pack temp, pack voltage

class EngineECU {
public:
    EngineECU();
    ~EngineECU();

    void run();
    void requestShutdown();

private:
    bool init();
    void cyclicTask();
    void resetSimulation();
    void evaluateFaults();
    void packEngineStatus(uint8_t* buf) const;
    void packFaultStatus(uint8_t* buf) const;
    void processIncomingFrames();
    void shutdown();

    // NOTE: declaration order matters -- uds_handler_ is constructed from
    // live_ and dtc_manager_, so those two must be declared BEFORE it.
    LiveData     live_;
    FaultManager dtc_manager_;
    UDSHandler   uds_handler_;

    ECUState          state_ = ECUState::INITIALIZING;
    std::atomic<bool> shutdown_requested_{false};
    int               can_socket_ = -1;

    // Simulation state
    uint64_t tick_count_ = 0;
    float    target_rpm_ = 800.0f;
    float    base_rpm_   = 0.0f;   // smoothed RPM before misfire noise is added
    bool     misfire_window_ = false;
    bool     bms_seen_       = false;   // no BMS fault until a BMS frame has arrived

    // Currently-true fault conditions
    bool overheat_active_     = false;  // P0117
    bool misfire_active_      = false;  // P0300
    bool voltage_high_active_ = false;  // P0563
    bool pack_overtemp_active_ = false; // P0A7E  (EV theme)
    bool pack_volt_low_active_ = false; // P0AFA  (EV theme)

    static constexpr int    CYCLIC_PERIOD_MS        = 100;   // one tick
    static constexpr int    MONITORING_CYCLE_TICKS  = 20;    // fault check every 2 s
    static constexpr double CYCLE_PERIOD_SEC        = 60.0;  // scripted drive cycle length
};
