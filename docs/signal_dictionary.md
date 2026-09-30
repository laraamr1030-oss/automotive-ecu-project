# Phase 2: CAN Signal Dictionary

All frames are standard 11-bit IDs, DLC as listed, **little-endian (Intel) bit numbering**: start bit 0 = LSB of byte 0.
`physical = raw * factor + offset`. Every row below was checked against the encode/decode code (`packEngineStatus`,
`packFaultStatus`, `packBCMStatus`, `packBmsStatus`, `TransmissionECU::cyclicTask`, `CANWorker::decodeAndEmit`).
All cyclic frames are sent every **100 ms**.

| CAN ID | Signal | Start bit | Length (bits) | Factor | Offset | Unit | Owning ECU |
| :--- | :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| **0x0C0** (DLC 8) | EngineSpeed | 0 | 16 | 0.25 | 0 | rpm | Engine ECU |
| 0x0C0 | CoolantTemp | 32 | 8 | 1 | -40 | °C | Engine ECU |
| 0x0C0 | ThrottlePos | 40 | 8 | 0.4 | 0 | % | Engine ECU |
| 0x0C0 | EngineRunning | 48 | 8 | 1 | 0 | 0/1 | Engine ECU |
| 0x0C0 | SystemVoltage (12 V) | 56 | 8 | 0.1 | 0 | V | Engine ECU |
| **0x0C1** (DLC 3) | FaultFlags (bit0 P0117, bit1 P0300, bit2 P0563, bit3 P0A7E, bit4 P0AFA) | 0 | 8 | 1 | 0 | bitfield | Engine ECU |
| 0x0C1 | StoredDTCCount | 8 | 8 | 1 | 0 | count | Engine ECU |
| 0x0C1 | CELRequest (warning lamp) | 16 | 8 | 1 | 0 | 0/1 | Engine ECU |
| **0x0D0** (DLC 8) | VehicleSpeed | 0 | 16 | 0.01 | 0 | km/h | Transmission ECU |
| 0x0D0 | CurrentGear | 16 | 8 | 1 | 0 | gear | Transmission ECU |
| **0x320** (DLC 8) | DoorFL | 0 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | DoorFR | 1 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | DoorRL | 2 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | DoorRR | 3 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | Hazard | 5 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | Ignition | 8 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | TurnLeft | 9 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | TurnRight | 10 | 1 | 1 | 0 | 0/1 | BCM |
| 0x320 | BatteryVolt | 16 | 8 | 0.1 | 0 | V | BCM |
| **0x350** (DLC 8) | BMS_SoC | 0 | 16 | 0.4 | 0 | % | BMS ECU (theme) |
| 0x350 | BMS_PackTemp | 16 | 8 | 0.5 | -40 | °C | BMS ECU (theme) |
| 0x350 | BMS_ChargingState | 24 | 8 | 1 | 0 | 0=discharge 1=charge | BMS ECU (theme) |
| 0x350 | BMS_PackVoltage | 32 | 16 | 0.1 | 0 | V | BMS ECU (theme) |
| **0x7E0** | UDS request (tester → ECU), ISO-TP | 0 | 64 | - | - | raw | Tester |
| **0x7E8** | UDS response (ECU → tester), ISO-TP | 0 | 64 | - | - | raw | Engine ECU |

## UDS Data Identifiers (SID 0x22) — one per signal

The DID data bytes are **big-endian** (UDS convention) and use the same factor/offset as the CAN signal.

| DID | Signal | Bytes | Factor | Offset | Unit |
| :--- | :--- | ---: | ---: | ---: | :--- |
| 0x0101 | EngineSpeed | 2 | 0.25 | 0 | rpm |
| 0x0102 | CoolantTemp | 1 | 1 | -40 | °C |
| 0x0103 | SystemVoltage (12 V) | 2 | 0.01 | 0 | V |
| 0x0104 | ThrottlePos | 1 | 0.4 | 0 | % |
| 0x0105 | EngineRunning | 1 | 1 | 0 | 0/1 |
| 0x0106 | FaultFlags | 1 | 1 | 0 | bitfield |
| 0x0107 | StoredDTCCount | 1 | 1 | 0 | count |
| 0x0201 | VehicleSpeed | 2 | 0.01 | 0 | km/h |
| 0x0202 | CurrentGear | 1 | 1 | 0 | gear |
| 0x0301 | Door/Hazard bits (0x320 byte 0) | 1 | 1 | 0 | bitfield |
| 0x0302 | Ignition/Turn bits (0x320 byte 1) | 1 | 1 | 0 | bitfield |
| 0x0303 | BCM BatteryVolt | 1 | 0.1 | 0 | V |
| 0x0401 | BMS_SoC | 2 | 0.4 | 0 | % |
| 0x0402 | BMS_PackTemp | 1 | 0.5 | -40 | °C |
| 0x0403 | BMS_ChargingState | 1 | 1 | 0 | 0/1 |
| 0x0404 | BMS_PackVoltage | 2 | 0.1 | 0 | V |

## Diagnostic Trouble Codes

| DTC | Bytes (high, low) | Condition (checked every 2 s) | Domain |
| :--- | :--- | :--- | :--- |
| P0117 | 0x01 0x17 | CoolantTemp > 105 °C | Engine |
| P0300 | 0x03 0x00 | RPM deviates > 180 rpm from target (misfire) | Engine |
| P0563 | 0x05 0x63 | SystemVoltage > 15.5 V | Engine / 12 V |
| **P0A7E** | 0x0A 0x7E | BMS pack temperature > 55 °C | **EV theme** |
| **P0AFA** | 0x0A 0xFA | BMS pack voltage < 320 V | **EV theme** |

Status byte lifecycle (ISO 14229-1): `0x05` pending → `0x8D` confirmed + warning lamp (after 2 consecutive failing
cycles) → `0x88` condition gone, DTC and lamp persist → removed only by UDS `0x14`. Freeze frame (RPM, coolant, 12 V,
SoC, pack temp, pack voltage) is captured once, when the DTC is first set, and read with SID 0x19 sub-function 0x04.
