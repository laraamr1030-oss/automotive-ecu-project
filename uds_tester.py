#!/usr/bin/env python3
"""
UDS diagnostic tester for the EV Engine/BMS ECU  (python-can, vcan0)

Exercises every service the ECU implements and every DID in the signal dictionary:
  SID 0x22 ReadDataByIdentifier      - all 16 DIDs, decoded with the dictionary's factor/offset
  SID 0x19 ReadDTCInformation        - sub 0x02 (by status mask) and sub 0x04 (freeze frame)
  SID 0x14 ClearDiagnosticInformation
  SID 0x11 ECUReset
  + negative responses (NRC 0x11, 0x12, 0x13-style, 0x31)
and watches a DTC go pending (0x05) -> confirmed (0x8D) live.

Usage:   python3 uds_tester.py                  # waits (default 90 s) for a fault to confirm
         python3 uds_tester.py --wait-timeout 0 # do not wait for faults
Record:  python3 uds_tester.py | tee docs/evidence/uds_transcript.txt
"""
from __future__ import annotations

import argparse
import sys
import time

import can

UDS_REQUEST_ID = 0x7E0    # tester -> ECU
UDS_RESPONSE_ID = 0x7E8   # ECU -> tester
ISOTP_TIMEOUT_S = 2.0

DID_ENGINE_RPM = 0x0101
DID_COOLANT_TEMP = 0x0102
DID_BATTERY_VOLTAGE = 0x0103

# (DID, name, unit, data length in bytes, decode(bytes) -> value)  -- factors/offsets = docs/signal_dictionary.md
def _u16(b): return (b[0] << 8) | b[1]
DID_TABLE = [
    (0x0101, "Engine speed",            "rpm",  2, lambda b: _u16(b) * 0.25),
    (0x0102, "Coolant temperature",     "degC", 1, lambda b: b[0] * 1.0 - 40.0),
    (0x0103, "12 V system voltage",     "V",    2, lambda b: _u16(b) * 0.01),
    (0x0104, "Throttle position",       "%",    1, lambda b: b[0] * 0.4),
    (0x0105, "Engine running",          "",     1, lambda b: b[0]),
    (0x0106, "Fault flags (bitfield)",  "",     1, lambda b: f"0x{b[0]:02X}"),
    (0x0107, "Stored DTC count",        "",     1, lambda b: b[0]),
    (0x0201, "Vehicle speed",           "km/h", 2, lambda b: _u16(b) * 0.01),
    (0x0202, "Current gear",            "",     1, lambda b: b[0]),
    (0x0301, "Door/hazard bits (BCM b0)", "",   1, lambda b: f"0b{b[0]:08b}"),
    (0x0302, "Ignition/turn bits (BCM b1)", "", 1, lambda b: f"0b{b[0]:08b}"),
    (0x0303, "BCM battery voltage",     "V",    1, lambda b: b[0] * 0.1),
    (0x0401, "BMS state of charge",     "%",    2, lambda b: _u16(b) * 0.4),
    (0x0402, "BMS pack temperature",    "degC", 1, lambda b: b[0] * 0.5 - 40.0),
    (0x0403, "BMS charging state",      "",     1, lambda b: b[0]),
    (0x0404, "BMS pack voltage",        "V",    2, lambda b: _u16(b) * 0.1),
]
DID_INFO = {row[0]: row for row in DID_TABLE}

DTC_BITS = [
    (0x01, "testFailed"), (0x02, "testFailedThisMonitoringCycle"), (0x04, "pendingDTC"),
    (0x08, "confirmedDTC"), (0x10, "testNotCompletedSinceLastClear"),
    (0x20, "testFailedSinceLastClear"), (0x40, "testNotCompletedThisMonitoringCycle"),
    (0x80, "warningIndicatorRequested"),
]
DTC_CATEGORY = {0: "P", 1: "C", 2: "B", 3: "U"}
NRC_NAMES = {0x11: "serviceNotSupported", 0x12: "subFunctionNotSupported",
             0x13: "incorrectMessageLength", 0x31: "requestOutOfRange"}

results: list[tuple[str, bool]] = []


def check(name: str, ok: bool) -> None:
    results.append((name, ok))
    print(f"  [{'PASS' if ok else 'FAIL'}] {name}")


# ---------------------------------------------------------------- transport
def send_request(bus: can.Bus, data: list[int]) -> None:
    padded = (data + [0] * 8)[:8]
    bus.send(can.Message(arbitration_id=UDS_REQUEST_ID, data=padded, is_extended_id=False))


def _send_flow_control(bus: can.Bus) -> None:
    send_request(bus, [0x30, 0x00, 0x00])           # ContinueToSend, BlockSize 0, STmin 0


def _recv_matching(bus: can.Bus, expect_id: int, timeout: float):
    """The bus also carries 0x0C0/0x0C1/0x0D0/0x320/0x350 -- discard everything but expect_id."""
    deadline = time.monotonic() + timeout
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            return None
        msg = bus.recv(timeout=remaining)
        if msg is None:
            return None
        if msg.arbitration_id == expect_id:
            return msg


def read_response(bus: can.Bus, timeout: float = ISOTP_TIMEOUT_S) -> bytes | None:
    """Full ISO-TP receive; returns SID+params with PCI bytes stripped."""
    msg = _recv_matching(bus, UDS_RESPONSE_ID, timeout)
    if msg is None:
        return None
    data = bytes(msg.data)
    pci = (data[0] >> 4) & 0x0F
    if pci == 0x0:                                   # Single Frame
        return data[1:1 + (data[0] & 0x0F)]
    if pci != 0x1:
        print(f"  [ISO-TP] unexpected PCI 0x{data[0]:02X}")
        return None
    total = ((data[0] & 0x0F) << 8) | data[1]        # First Frame
    payload = bytearray(data[2:8])
    print(f"    [ISO-TP] First Frame, total {total} bytes -> sending Flow Control (CTS)")
    _send_flow_control(bus)
    expected = 1
    while len(payload) < total:
        cf = _recv_matching(bus, UDS_RESPONSE_ID, timeout)
        if cf is None:
            print("    [ISO-TP] timed out waiting for Consecutive Frame")
            return None
        d = bytes(cf.data)
        if (d[0] & 0xF0) != 0x20:
            print(f"    [ISO-TP] expected Consecutive Frame, got 0x{d[0]:02X}")
            return None
        if (d[0] & 0x0F) != expected:
            print(f"    [ISO-TP] sequence warning: expected {expected}, got {d[0] & 0x0F}")
        payload.extend(d[1:8])
        expected = (expected + 1) % 16
    return bytes(payload[:total])


def is_negative(resp: bytes | None, sid: int, nrc: int) -> bool:
    return resp is not None and len(resp) >= 3 and resp[0] == 0x7F and resp[1] == sid and resp[2] == nrc


# ---------------------------------------------------------------- services
def read_did(bus: can.Bus, did: int) -> bytes | None:
    send_request(bus, [0x03, 0x22, did >> 8, did & 0xFF])
    return read_response(bus)


def read_dtcs(bus: can.Bus, mask: int = 0xFF) -> list[tuple[int, int, int]] | None:
    send_request(bus, [0x03, 0x19, 0x02, mask])
    r = read_response(bus)
    if r is None or len(r) < 3 or r[0] != 0x59:
        return None
    return [(r[i], r[i + 1], r[i + 2]) for i in range(3, len(r) - 2, 3)]


def read_freeze_frame(bus: can.Bus, high: int, low: int) -> bytes | None:
    send_request(bus, [0x04, 0x19, 0x04, high, low, 0x01])
    return read_response(bus)


def clear_dtcs(bus: can.Bus) -> bool:
    send_request(bus, [0x04, 0x14, 0xFF, 0xFF, 0xFF])
    r = read_response(bus)
    return r is not None and r[:1] == b"\x54"


def ecu_reset(bus: can.Bus, sub: int = 0x01) -> bytes | None:
    send_request(bus, [0x02, 0x11, sub])
    return read_response(bus)


# ---------------------------------------------------------------- decoding
def dtc_name(high: int, low: int) -> str:
    return f"{DTC_CATEGORY[(high >> 6) & 3]}{(high >> 4) & 3:X}{high & 0x0F:X}{(low >> 4) & 0x0F:X}{low & 0x0F:X}"


def status_text(status: int) -> str:
    names = [n for m, n in DTC_BITS if status & m]
    return f"0x{status:02X} ({'|'.join(names) if names else 'no bits'})"


def decode_freeze_frame(p: bytes) -> dict | None:
    if len(p) < 7 or p[0] != 0x59 or p[1] != 0x04:
        return None
    out = {"high": p[2], "low": p[3], "status": p[4], "record": p[5], "values": []}
    off = 7
    for _ in range(p[6]):
        did = (p[off] << 8) | p[off + 1]
        if did not in DID_INFO:
            break
        _, name, unit, length, fn = DID_INFO[did]
        out["values"].append((did, name, unit, fn(p[off + 2:off + 2 + length])))
        off += 2 + length
    return out


# ---------------------------------------------------------------- test steps
def step_read_all_dids(bus):
    print("\n[1] SID 0x22 ReadDataByIdentifier -- every DID in the signal dictionary")
    for did, name, unit, length, fn in DID_TABLE:
        r = read_did(bus, did)
        ok = (r is not None and len(r) == 3 + length and r[0] == 0x62 and (r[1] << 8 | r[2]) == did)
        if ok:
            val = fn(r[3:])
            shown = f"{val:.2f}" if isinstance(val, float) else str(val)
            print(f"  DID 0x{did:04X}  {name:<28} = {shown} {unit}")
        check(f"DID 0x{did:04X} {name}", ok)


def step_negative_responses(bus):
    print("\n[2] Negative responses (7F <SID> <NRC>)")
    r = read_did(bus, 0x9999)
    check("unknown DID 0x9999 -> 7F 22 31 requestOutOfRange", is_negative(r, 0x22, 0x31))
    send_request(bus, [0x02, 0x27, 0x01])
    check("unsupported SID 0x27 -> 7F 27 11 serviceNotSupported", is_negative(read_response(bus), 0x27, 0x11))
    check("ECUReset sub-function 0x02 -> 7F 11 12 subFunctionNotSupported",
          is_negative(ecu_reset(bus, 0x02), 0x11, 0x12))
    send_request(bus, [0x03, 0x19, 0x07, 0xFF])
    check("ReadDTC sub-function 0x07 -> 7F 19 12 subFunctionNotSupported",
          is_negative(read_response(bus), 0x19, 0x12))
    check("freeze frame of non-existent DTC -> 7F 19 31", is_negative(read_freeze_frame(bus, 0x09, 0x99), 0x19, 0x31))


def step_wait_for_confirmed(bus, timeout_s: float) -> bool:
    print(f"\n[3] Watching DTC lifecycle live (up to {timeout_s:.0f} s; faults appear on the ECU's 60 s drive cycle)")
    t0 = time.monotonic()
    last = None
    saw_pending = False
    while time.monotonic() - t0 < timeout_s:
        recs = read_dtcs(bus, 0xFF)
        if recs is None:
            print("  no answer from ECU -- is engine_ecu running?")
            return False
        state = tuple(sorted(recs))
        if state != last:
            print(f"  t+{time.monotonic() - t0:5.1f}s  " +
                  ("no DTCs" if not recs else "; ".join(f"{dtc_name(h, l)}={status_text(s)}" for h, l, s in recs)))
            last = state
        if any(s == 0x05 for _, _, s in recs):
            saw_pending = True
        if any(s & 0x08 for _, _, s in recs):
            check("a DTC reached confirmed (bit3) after being pending" if saw_pending
                  else "a DTC is confirmed (was already past pending when we started)", True)
            return True
        time.sleep(1.0)
    check("a DTC confirmed within the wait window", False)
    return False


def step_dtc_services(bus):
    print("\n[4] SID 0x19 ReadDTCInformation")
    print("  sub 0x02, mask 0xFF (all stored DTCs):")
    all_dtcs = read_dtcs(bus, 0xFF) or []
    for h, l, s in all_dtcs:
        print(f"    {dtc_name(h, l)}  status={status_text(s)}")
    check("reportDTCByStatusMask(0xFF) returned records", len(all_dtcs) > 0)

    print("  sub 0x02, mask 0x08 (confirmed only):")
    confirmed = read_dtcs(bus, 0x08) or []
    for h, l, s in confirmed:
        print(f"    {dtc_name(h, l)}  status={status_text(s)}")
    check("mask 0x08 returns only confirmed DTCs", all(s & 0x08 for _, _, s in confirmed) and len(confirmed) > 0)

    print("  sub 0x04 (freeze frame) for every stored DTC:")
    ok_all = True
    for h, l, _ in all_dtcs:
        ff = decode_freeze_frame(read_freeze_frame(bus, h, l) or b"")
        if ff is None:
            ok_all = False
            print(f"    {dtc_name(h, l)}: no/invalid freeze frame")
            continue
        print(f"    {dtc_name(h, l)}  snapshot #{ff['record']}  status={status_text(ff['status'])}")
        for did, name, unit, val in ff["values"]:
            print(f"        0x{did:04X} {name:<24} {val:.2f} {unit}")
        ok_all = ok_all and len(ff["values"]) == 6
    check("freeze frame (6 identifiers) readable for every stored DTC", ok_all and len(all_dtcs) > 0)


def step_clear_and_reset(bus):
    print("\n[5] SID 0x14 ClearDiagnosticInformation")
    check("ClearDiagnosticInformation -> positive response 0x54", clear_dtcs(bus))
    after = read_dtcs(bus, 0xFF)
    print("  DTCs right after clear:", "none" if not after else after)
    check("no confirmed DTC remains right after clear", after is not None and not any(s & 0x08 for _, _, s in after))

    print("\n[6] SID 0x11 ECUReset (hardReset)")
    r = ecu_reset(bus, 0x01)
    check("ECUReset -> positive response 51 01", r == b"\x51\x01")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--channel", default="vcan0")
    ap.add_argument("--wait-timeout", type=float, default=90.0,
                    help="seconds to wait for a DTC to confirm (0 = skip DTC tests)")
    args = ap.parse_args()

    print("=== UDS Diagnostic Tester (EV theme: Engine + BMS) ===")
    print(f"Connecting to {args.channel} ...")
    bus = can.Bus(interface="socketcan", channel=args.channel)
    try:
        step_read_all_dids(bus)
        step_negative_responses(bus)
        if args.wait_timeout > 0:
            if step_wait_for_confirmed(bus, args.wait_timeout):
                step_dtc_services(bus)
        step_clear_and_reset(bus)
    finally:
        bus.shutdown()

    failed = [n for n, ok in results if not ok]
    print(f"\n=== {len(results) - len(failed)}/{len(results)} checks passed ===")
    for n in failed:
        print(f"  FAILED: {n}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
