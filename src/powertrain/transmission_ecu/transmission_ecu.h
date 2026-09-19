#pragma once
#include <cstdint>
#include <atomic>

constexpr uint32_t TRANS_STATUS_ID  = 0x0D0;
constexpr uint32_t ENGINE_STATUS_ID = 0x0C0;
constexpr int      CYCLIC_PERIOD_MS = 100;

enum class Gear : uint8_t { PARK = 0, REVERSE = 1, NEUTRAL = 2, DRIVE = 3 };
enum class ECUState { INITIALIZING, RUNNING, FAULT, SHUTTING_DOWN };

class TransmissionECU {
public:
    TransmissionECU();
    ~TransmissionECU();
    void run();
    void requestShutdown();

private:
    bool init();
    void cyclicTask();
    void packTransmissionStatus(uint8_t* buf) const;
    void shutdown();

    // Simulation state
    Gear  gear_        = Gear::DRIVE;
    float vehicle_speed_ = 0.0f;   // km/h
    float engine_rpm_    = 0.0f;   // received from Engine ECU

    ECUState state_ = ECUState::INITIALIZING;
    std::atomic<bool> shutdown_requested_{false};
    int can_socket_ = -1;
};
