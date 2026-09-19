#pragma once
#include <cstdint>
#include <atomic>

constexpr uint32_t ENGINE_STATUS_ID = 0x0C0;
constexpr int      CYCLIC_PERIOD_MS = 100;

enum class ECUState { INITIALIZING, RUNNING, FAULT, SHUTTING_DOWN };

class EngineECU {
public:
    EngineECU();
    ~EngineECU();

    void run();       // main loop — blocks until shutdown
    void requestShutdown();

    // Opt-in only (see main.cpp's --demo flag). Default behavior — rpm_ ramps
    // once toward a fixed target_rpm_ and then holds — is exactly what
    // Exercise 1 of Session 07's lab.md is graded against; leave it alone.
    // Demo mode instead keeps target_rpm_ sweeping forever, which is what a
    // live dashboard (this session) needs so its gauges never go static no
    // matter when it connects relative to this process starting.
    void setDemoMode(bool enabled) { demo_mode_ = enabled; }

private:
    bool init();      // open CAN socket, bind to vcan0
    void cyclicTask();        // update simulation, pack + send CAN frame
    void packEngineStatus(uint8_t* buf) const; // encode signals into 8 bytes
    void shutdown();  // close CAN socket

    // Simulation state
    float rpm_         = 0.0f;
    float target_rpm_  = 2000.0f;
    float coolant_temp_= 25.0f;   // degrees Celsius
    float throttle_    = 20.0f;   // percent

    // Demo mode: repeating idle -> rev -> idle drive cycle (see cyclicTask()).
    bool     demo_mode_ = false;
    uint64_t demo_tick_ = 0;
    static constexpr float DEMO_IDLE_RPM = 800.0f;
    static constexpr float DEMO_PEAK_RPM = 4000.0f;
    static constexpr int   DEMO_CYCLE_MS = 20000; // one full idle->rev->idle sweep

    // ECU state
    ECUState state_ = ECUState::INITIALIZING;
    std::atomic<bool> shutdown_requested_{false};

    // CAN socket
    int can_socket_ = -1;
};
