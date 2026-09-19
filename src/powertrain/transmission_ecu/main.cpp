#include <iostream>
#include <csignal>
#include "transmission_ecu.h"

static TransmissionECU* g_ecu = nullptr;

void signalHandler(int) {
    if (g_ecu) g_ecu->requestShutdown();
}

int main() {
    std::signal(SIGINT, signalHandler);
    TransmissionECU ecu;
    g_ecu = &ecu;
    std::cout << "[main] Starting Transmission ECU.\n";
    ecu.run();
    return 0;
}
