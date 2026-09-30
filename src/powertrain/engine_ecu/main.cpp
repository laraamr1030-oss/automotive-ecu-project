#include <csignal>
#include <iostream>
#include "engine_ecu.h"

static EngineECU* g_ecu = nullptr;

void signalHandler(int) {
    std::cout << "\n[main] Shutdown signal received.\n";
    if (g_ecu) g_ecu->requestShutdown();
}

int main() {
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    EngineECU ecu;
    g_ecu = &ecu;

    std::cout << "[main] Starting Engine ECU (UDS + fault simulation). Ctrl+C to stop.\n";
    ecu.run();
    return 0;
}
