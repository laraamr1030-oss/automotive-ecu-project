#include "bms_ecu.h"
#include <iostream>
#include <csignal>

BMSEcu* g_bms_ecu = nullptr;

void signalHandler(int signum) {
    std::cout << "\n[Main] Shutdown signal received. Stopping BMS ECU...\n";
    if (g_bms_ecu) {
        g_bms_ecu->requestShutdown();
    }
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    BMSEcu bms;
    g_bms_ecu = &bms;

    bms.run();
    return 0;
}
