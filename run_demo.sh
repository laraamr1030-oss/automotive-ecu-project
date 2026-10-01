#!/usr/bin/env bash
# Starts the whole stack on vcan0: 4 ECU processes + Qt dashboard.
# Usage: ./run_demo.sh        (Ctrl+C stops everything)
set -e
cd "$(dirname "$0")"
BIN=build/bin

if [ ! -x "$BIN/engine_ecu" ]; then
    echo "Build first:  cmake -S . -B build && cmake --build build -j"
    exit 1
fi

# vcan0 (Session 6 setup) -- created only if it does not exist yet
if ! ip link show vcan0 >/dev/null 2>&1; then
    sudo modprobe vcan
    sudo ip link add dev vcan0 type vcan
fi
sudo ip link set up vcan0

pids=()
cleanup() { kill "${pids[@]}" 2>/dev/null || true; }
trap cleanup EXIT INT TERM

"$BIN/engine_ecu"       > /tmp/engine_ecu.log       2>&1 & pids+=($!)
"$BIN/transmission_ecu" > /tmp/transmission_ecu.log 2>&1 & pids+=($!)
"$BIN/bcm"              > /tmp/bcm.log              2>&1 & pids+=($!)
"$BIN/bms_ecu"          > /tmp/bms_ecu.log          2>&1 & pids+=($!)
echo "ECUs started (logs: /tmp/*_ecu.log, /tmp/bcm.log)"

if [ -x "$BIN/dashboard" ]; then
    "$BIN/dashboard" & pids+=($!)
    echo "Dashboard started."
else
    echo "dashboard binary not found (Qt missing?) - running ECUs only."
fi

echo "Full stack running on vcan0. Press Ctrl+C to stop."
wait
