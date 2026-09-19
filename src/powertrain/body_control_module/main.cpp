#include <iostream>
#include <csignal>
#include "bcm.h"

static BCM* g_bcm = nullptr;

void signalHandler(int) {
    if (g_bcm) g_bcm->requestShutdown();
}

int main() {
    std::signal(SIGINT, signalHandler);
    BCM bcm;
    g_bcm = &bcm;
    std::cout << "[main] Starting Body Control Module.\n";
    bcm.run();
    return 0;
}
