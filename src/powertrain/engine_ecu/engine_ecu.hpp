#pragma once

class EngineECU {
public:
    EngineECU();
    ~EngineECU();
    bool init();
    void run();
    void requestShutdown();
    void shutdown();

private:
    int s = -1;
};
