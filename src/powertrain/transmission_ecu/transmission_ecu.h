#pragma once

#include <atomic>
#include <cstdint>
#include <string>

// Transmission ECU: listens to the engine's 0x0C0 (RPM), derives vehicle
// speed and gear, and broadcasts them on 0x0D0 every 100 ms.
class TransmissionECU {
public:
    TransmissionECU();
    ~TransmissionECU();

    bool initSocketCAN(const std::string& interface_name);
    void run();
    void requestShutdown();
    void shutdown();

private:
    void cyclicTask();

    int               socket_fd_;
    std::atomic<bool> running_;
    uint8_t           current_gear_;
    float             rpm_;         // last RPM received from 0x0C0
    float             speed_kmh_;   // simulated vehicle speed

    static constexpr uint32_t ENGINE_STATUS_ID = 0x0C0;
    static constexpr uint32_t TRANSMISSION_STATUS_ID = 0x0D0;
    static constexpr int      CYCLIC_PERIOD_MS = 100;
};
