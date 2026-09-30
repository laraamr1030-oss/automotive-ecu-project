# Member 2 - apply steps
cd <repo>
git checkout -b member2-diagnostics
cp -r member2_diagnostics/* .                         # new/replacement files, same paths
git rm engine_ecu.cpp src/powertrain/engine_ecu/engine_ecu.hpp   # orphaned/duplicate files
# (Member 1's transmission + BMS fixes must be merged for the full demo)
mkdir -p build
g++ -std=c++17 -Isrc/powertrain/engine_ecu tests/test_diagnostics_offline.cpp src/powertrain/engine_ecu/uds_handler.cpp -o build/test_diag -pthread && ./build/test_diag
cmake -S src/powertrain/engine_ecu -B build/engine && cmake --build build/engine -j
git add -A && git commit -m "Diagnostics: FaultManager lifecycle, UDS 0x22/0x19/0x14/0x11, BMS DTCs, tester"
