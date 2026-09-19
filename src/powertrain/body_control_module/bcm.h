#pragma once
#include <cstdint>
#include <atomic>

constexpr uint32_t BCM_STATUS_ID = 0x320;
enum class ECUState { INITIALIZING, RUNNING, FAULT, SHUTTING_DOWN };

struct BCMState {
    bool door_fl  = false;
    bool door_fr  = false;
    bool door_rl  = false;
    bool door_rr  = false;
    bool hazard   = false;
    bool ignition = true;
    bool turn_left  = false;
    bool turn_right = false;
    float battery_volt = 14.0f;
};

class BCM {
public:
    BCM();
    ~BCM();
    void run();
    void requestShutdown();

private:
    bool init();
    void cyclicTask();
    void runScenario();
    void packBCMStatus(uint8_t* buf) const;
    void shutdown();

    BCMState state_{};
    int scenario_tick_ = 0;

    ECUState ecu_state_ = ECUState::INITIALIZING;
    std::atomic<bool> shutdown_requested_{false};
    int can_socket_ = -1;
};
