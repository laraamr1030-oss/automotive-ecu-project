#ifndef BMS_ECU_H
#define BMS_ECU_H

#include <cstdint>
#include <atomic>

enum class ECUState {
    UNINITIALIZED,
    INITIALIZING,
    RUNNING,
    FAULT,
    SHUTTING_DOWN
};

class BMSEcu {
public:
    BMSEcu();
    ~BMSEcu();

    void run();
    void requestShutdown();

    // Getters for local state or testing
    float getSoC() const { return soc_; }
    float getPackTemp() const { return pack_temp_; }
    uint8_t getChargingState() const { return charging_state_; }
    float getPackVoltage() const { return pack_voltage_; }

private:
    bool init();
    void cyclicTask();
    void packBmsStatus(uint8_t* buf) const;
    void shutdown();

    int can_socket_ = -1;
    std::atomic<bool> shutdown_requested_{false};
    ECUState state_ = ECUState::UNINITIALIZED;

    // Simulated metrics
    float soc_ = 85.0f;           // State of charge percentage
    float pack_temp_ = 25.0f;     // Pack temperature in Celsius
    float pack_voltage_ = 350.0f; // Voltage in Volts
    uint8_t charging_state_ = 0;  // 0: Discharging, 1: Charging

    static constexpr uint32_t BMS_STATUS_ID = 0x350;
    static constexpr int CYCLIC_PERIOD_MS = 100;
};

#endif // BMS_ECU_H
