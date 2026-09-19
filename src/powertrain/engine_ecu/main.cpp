#include <iostream>
#include <csignal>
#include <string>
#include "engine_ecu.h"

static EngineECU* g_ecu = nullptr;

void signalHandler(int) {
    std::cout << "\n[main] Shutdown signal received.\n";
    if (g_ecu) g_ecu->requestShutdown();
}

int main(int argc, char** argv) {
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    EngineECU ecu;
    g_ecu = &ecu;

    // --demo: keep RPM sweeping idle<->redline forever instead of settling
    // on a fixed target, so a dashboard connecting late still sees movement.
    // Only meant for live-dashboard demos (this session) — not used by
    // Session 07's own lab exercises, whose expected output assumes the
    // default fixed-target behavior.
    bool demo_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--demo") demo_mode = true;
    }
    if (demo_mode) {
        ecu.setDemoMode(true);
        std::cout << "[main] Demo mode enabled: RPM will sweep idle<->redline continuously.\n";
    }

    std::cout << "[main] Starting Engine ECU. Press Ctrl+C to stop.\n";
    ecu.run();

    return 0;
}
