#pragma once

#include <string>
#include <cstdint>

class TransmissionECU {
public:
    TransmissionECU();
    ~TransmissionECU();

    bool initSocketCAN(const std::string& interface_name);
    void run();
    void requestShutdown();
    void shutdown();

private:
    int socket_fd_;
    bool running_;
    uint8_t current_gear_;
};