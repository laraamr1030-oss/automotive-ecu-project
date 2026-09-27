#!/usr/bin/env python3
"""
Session 14 Demo — UDS Diagnostic Tester for the Fault Dashboard Demo

Same real ISO-TP receiver (read_response()) as Session 13's demo tester,
reused verbatim here, now driving ReadDTCInformation (SID 0x19, both
sub-function 0x02 reportDTCByStatusMask and 0x04
reportDTCSnapshotRecordByDTCNumber -- the freeze frame) and
ClearDiagnosticInformation (SID 0x14) against the fault Engine ECU's live
FaultManager instead of an empty placeholder DTC list.

Run this at any point while engine_ecu_fault is running (standalone, or via
run_demo.sh alongside the dashboard) to watch the DTC status byte evolve:
pending (0x05) -> confirmed (0x8D) -> persists after the condition clears ->
fully removed only after an explicit clear (content.md Section 2.3 / Pitfall 1).
Also reads back each active DTC's freeze frame -- the RPM/coolant
temp/battery voltage captured at the instant it was first set (content.md's
freeze-frame learning objective).
"""

import time

import can

UDS_REQUEST_ID   = 0x7E0   # tester -> ECU
UDS_RESPONSE_ID  = 0x7E8   # ECU -> tester
ENGINE_STATUS_ID = 0x0C0   # ECU's unrelated 100ms cyclic broadcast
FAULT_STATUS_ID  = 0x0C1   # ECU's unrelated 100ms fault-status broadcast

DID_ENGINE_RPM    = 0x0101
DID_COOLANT_TEMP  = 0x0102
DID_VEHICLE_SPEED = 0x0201
DID_BATTERY_VOLTAGE = 0x0103

ISOTP_TIMEOUT_S = 2.0

# ISO 14229-1 DTC status byte bits (content.md Section 2.2) -- mirrors
# fault_manager.h's constants so the tester can print human-readable status.
DTC_BITS = [
    (0x01, "testFailed"),
    (0x02, "testFailedThisMonitoringCycle"),
    (0x04, "pendingDTC"),
    (0x08, "confirmedDTC"),
    (0x10, "testNotCompletedSinceLastClear"),
    (0x20, "testFailedSinceLastClear"),
    (0x40, "testNotCompletedThisMonitoringCycle"),
    (0x80, "warningIndicatorRequested"),
]

DTC_CATEGORY = {0: "P", 1: "C", 2: "B", 3: "U"}  # bits7-6 of the high byte


def send_request(bus: can.Bus, data: list[int]) -> None:
    padded = (data + [0] * 8)[:8]
    msg = can.Message(arbitration_id=UDS_REQUEST_ID, data=padded, is_extended_id=False)
    bus.send(msg)


def _send_flow_control(bus: can.Bus, block_size: int = 0, stmin: int = 0) -> None:
    """CTS (ContinueToSend), the only Flow Status this demo ever sends."""
    send_request(bus, [0x30, block_size, stmin, 0, 0, 0, 0, 0])


def _recv_matching(bus: can.Bus, expect_id: int, timeout: float) -> can.Message | None:
    """The bus also carries 0x0C0 and 0x0C1 cyclic broadcasts -- keep
    draining frames, discarding anything that isn't `expect_id`, until we
    find a match or the deadline elapses."""
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
        # else: unrelated cyclic traffic -- keep waiting.


def read_response(bus: can.Bus, timeout: float = ISOTP_TIMEOUT_S) -> bytes | None:
    """Full ISO-TP receive (Session 13's demo, reused verbatim). Returns the
    reassembled SID+params payload with any PCI byte(s) already stripped."""
    msg = _recv_matching(bus, UDS_RESPONSE_ID, timeout)
    if msg is None:
        return None

    data = bytes(msg.data)
    pci_type = (data[0] >> 4) & 0x0F

    if pci_type == 0x0:
        length = data[0] & 0x0F
        return data[1:1 + length]

    if pci_type != 0x1:
        print(f"  [ISO-TP] Unexpected leading PCI 0x{data[0]:02X} (expected Single or First Frame)")
        return None

    total_len = ((data[0] & 0x0F) << 8) | data[1]
    payload = bytearray(data[2:8])
    print(f"  [ISO-TP] First Frame: total length {total_len}, got {len(payload)} bytes so far")

    _send_flow_control(bus, block_size=0, stmin=0)
    print("  [ISO-TP] Sent Flow Control: CTS, BlockSize=0, STmin=0")

    expected_seq = 1
    while len(payload) < total_len:
        cf_msg = _recv_matching(bus, UDS_RESPONSE_ID, timeout)
        if cf_msg is None:
            print("  [ISO-TP] Timed out waiting for a Consecutive Frame")
            return None
        cf = bytes(cf_msg.data)
        if (cf[0] & 0xF0) != 0x20:
            print(f"  [ISO-TP] Expected a Consecutive Frame, got PCI 0x{cf[0]:02X}")
            return None
        seq = cf[0] & 0x0F
        if seq != expected_seq:
            print(f"  [ISO-TP] Warning: expected sequence {expected_seq}, got {seq}")
        payload.extend(cf[1:8])
        expected_seq = (expected_seq + 1) % 16

    return bytes(payload[:total_len])


def read_data_by_id(bus: can.Bus, did: int) -> bytes | None:
    did_high = (did >> 8) & 0xFF
    did_low  =  did       & 0xFF
    send_request(bus, [0x03, 0x22, did_high, did_low, 0, 0, 0, 0])
    return read_response(bus)


def read_dtcs(bus: can.Bus) -> bytes | None:
    """SID 0x19, sub-function 0x02 (reportDTCByStatusMask), mask 0xFF."""
    send_request(bus, [0x03, 0x19, 0x02, 0xFF, 0, 0, 0, 0])
    return read_response(bus)


def read_freeze_frame(bus: can.Bus, high: int, low: int) -> bytes | None:
    """SID 0x19, sub-function 0x04 (reportDTCSnapshotRecordByDTCNumber) --
    this demo's own wire format (content.md states the freeze-frame
    objective but never specifies one). Trailing record-number byte is
    accepted by the ECU but ignored, since this demo only keeps one
    snapshot per DTC."""
    send_request(bus, [0x04, 0x19, 0x04, high, low, 0x01, 0, 0])
    return read_response(bus)


def decode_freeze_frame(payload: bytes) -> dict | None:
    if len(payload) < 7 or payload[0] != 0x59 or payload[1] != 0x04:
        return None
    high, low, status, record_num, num_ids = payload[2], payload[3], payload[4], payload[5], payload[6]
    offset = 7
    values = {}
    for _ in range(num_ids):
        did = (payload[offset] << 8) | payload[offset + 1]
        if did == DID_ENGINE_RPM:
            raw = (payload[offset + 2] << 8) | payload[offset + 3]
            values["rpm"] = decode_rpm(bytes([0x62]) + payload[offset:offset + 4])
            offset += 4
        elif did == DID_COOLANT_TEMP:
            values["coolant_temp"] = payload[offset + 2] - 40.0
            offset += 3
        elif did == 0x0103:  # DID_BATTERY_VOLTAGE
            values["battery_voltage"] = payload[offset + 2] * 0.1
            offset += 3
        else:
            break  # unknown DID -- stop, rather than misparse the rest
    return {"high": high, "low": low, "status": status, "record": record_num, "values": values}


def print_freeze_frame(bus: can.Bus, high: int, low: int) -> None:
    data = read_freeze_frame(bus, high, low)
    if data is None:
        print(f"  Timeout -- no response from ECU")
        return
    ff = decode_freeze_frame(data)
    if ff is None:
        print(f"  Unexpected payload: {' '.join(f'{b:02X}' for b in data)}")
        return
    name = decode_dtc_name(ff["high"], ff["low"])
    print(f"  {name}  snapshot #{ff['record']}  status={decode_dtc_status(ff['status'])}")
    for key, val in ff["values"].items():
        print(f"    {key}: {val:.1f}")


def clear_dtcs(bus: can.Bus) -> bool:
    send_request(bus, [0x04, 0x14, 0xFF, 0xFF, 0xFF, 0, 0, 0])
    resp = read_response(bus)
    return resp is not None and len(resp) >= 1 and resp[0] == 0x54


def ecu_reset(bus: can.Bus) -> bool:
    send_request(bus, [0x02, 0x11, 0x01, 0, 0, 0, 0, 0])
    resp = read_response(bus)
    return resp is not None and len(resp) >= 1 and resp[0] == 0x51


# --- Decoders: `payload` here is SID + params, PCI already stripped. ---

def decode_rpm(payload: bytes) -> float | None:
    if len(payload) < 5 or payload[0] != 0x62:
        return None
    raw = (payload[3] << 8) | payload[4]
    return raw * 0.25


def decode_temperature(payload: bytes) -> float | None:
    if len(payload) < 4 or payload[0] != 0x62:
        return None
    raw = payload[3]
    return raw - 40.0  # factor=1.0, offset=-40 -- matches engine_ecu.cpp


def decode_dtc_name(high: int, low: int) -> str:
    """content.md Section 1.2: bits7-6 of the high byte select the P/C/B/U
    category; the remaining nibbles are the four digits of the code."""
    category = DTC_CATEGORY[(high >> 6) & 0x03]
    digit1 = (high >> 4) & 0x03
    digit2 = high & 0x0F
    digit3 = (low >> 4) & 0x0F
    digit4 = low & 0x0F
    return f"{category}{digit1:X}{digit2:X}{digit3:X}{digit4:X}"


def decode_dtc_status(status: int) -> str:
    names = [name for mask, name in DTC_BITS if status & mask]
    bits = "|".join(names) if names else "no bits set"
    return f"0x{status:02X} ({bits})"


def decode_dtcs(payload: bytes) -> list[tuple[int, int, int]] | None:
    """Returns a list of (high, low, status) 3-byte DTC records."""
    if len(payload) < 3 or payload[0] != 0x59:
        return None
    records = []
    for i in range(3, len(payload) - 2, 3):
        records.append((payload[i], payload[i + 1], payload[i + 2]))
    return records


def print_dtcs(bus: can.Bus) -> None:
    data = read_dtcs(bus)
    if data is None:
        print("  Timeout -- no response from ECU")
        return
    records = decode_dtcs(data)
    if records is None:
        print(f"  Unexpected payload: {' '.join(f'{b:02X}' for b in data)}")
        return
    if not records:
        print("  No active DTCs.")
        return
    for high, low, status in records:
        name = decode_dtc_name(high, low)
        print(f"  {name}  (0x{high:02X} 0x{low:02X})  status={decode_dtc_status(status)}")


def main():
    print("=== UDS Fault Diagnostic Tool (Session 14 Demo) ===")
    print("Connecting to vcan0...\n")

    bus = can.Bus(interface='socketcan', channel='vcan0')

    try:
        print("[Read DID 0x0101 - Engine RPM]")
        data = read_data_by_id(bus, DID_ENGINE_RPM)
        if data:
            rpm = decode_rpm(data)
            print(f"  RPM: {rpm:.0f}" if rpm is not None else "  Error decoding RPM")
        else:
            print("  Timeout -- no response from ECU")
        print()

        print("[Read DID 0x0102 - Coolant Temperature]")
        data = read_data_by_id(bus, DID_COOLANT_TEMP)
        if data:
            temp = decode_temperature(data)
            print(f"  Temperature: {temp:.1f}°C" if temp is not None else "  Error decoding temp")
        else:
            print("  Timeout -- no response from ECU")
        print()

        print("[Read DID 0x0201 - Vehicle Speed]")
        data = read_data_by_id(bus, DID_VEHICLE_SPEED)

        if data:
            raw = (data[-2] << 8) | data[-1]
            speed = raw * 0.01
            print(f"  Vehicle Speed: {speed:.2f} km/h")
        else:
            print("  Timeout -- no response from ECU")

        print()

        print("[Read DID 0x0103 - Battery Voltage]")
        data = read_data_by_id(bus, DID_BATTERY_VOLTAGE)

        if data:
            raw = (data[-2] << 8) | data[-1]
            voltage = raw * 0.01
            print(f"  Battery Voltage: {voltage:.2f} V")
        else:
            print("  Timeout -- no response from ECU")

        print()

        print("[ReadDTCInformation (SID 0x19) -- current DTCs]")
        print_dtcs(bus)
        print()

        print("[Waiting 3s for the next monitoring cycle -- watch for pending -> confirmed promotion]")
        time.sleep(3)
        print("[ReadDTCInformation (SID 0x19) -- after waiting]")
        print_dtcs(bus)
        print()

        print("[ReadDTCInformation sub-function 0x04 -- freeze frame per active DTC]")
        data = read_dtcs(bus)
        records = decode_dtcs(data) if data else None
        if records:
            for high, low, _status in records:
                print_freeze_frame(bus, high, low)
        else:
            print("  No active DTCs to snapshot.")
        print()

        print("[ClearDiagnosticInformation (SID 0x14)]")
        ok = clear_dtcs(bus)
        print("  Response: 54 -> DTCs cleared successfully" if ok else "  Failed to clear DTCs")
        print()

        print("[ReadDTCInformation (SID 0x19) -- after clearing, should be empty]")
        print_dtcs(bus)

        print()
        print("[ECUReset (SID 0x11) -- hard reset]")
        ok = ecu_reset(bus)
        print("  Response: 51 01 -> ECU reset requested successfully"
            if ok else
            "  Failed to reset ECU")

    finally:
        bus.shutdown()


if __name__ == "__main__":
    main()
