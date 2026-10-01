#!/usr/bin/env bash
# Saves the Phase 3 proof: every CAN ID seen on vcan0 at the same time.
# Run while ./run_demo.sh (or the 4 ECUs) is running.
set -e
cd "$(dirname "$0")"
mkdir -p docs/evidence
echo "Capturing 10 s of vcan0 ..."
timeout 10 candump -L vcan0 > docs/evidence/candump_all_ids.log || true
{
  echo "CAN IDs seen on vcan0 during a 10 s capture (count  ID):"
  awk '{split($3,a,"#"); print a[1]}' docs/evidence/candump_all_ids.log | sort | uniq -c
} | tee docs/evidence/candump_summary.txt
