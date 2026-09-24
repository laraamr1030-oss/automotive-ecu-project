#pragma once

#include <string>
#include <cstdint>

class EngineECU {
public:
    EngineECU();
    ~EngineECU();

    bool initSocketCAN(const std::string& interface_name);
    void run();
    void requestShutdown();
    void setDemoMode(bool enable);

private:
    int socket_fd_;
    bool running_;
    bool demo_mode_;
};