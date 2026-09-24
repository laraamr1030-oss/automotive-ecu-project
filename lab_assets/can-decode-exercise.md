# Exercise 1 — Decoding CAN Frames

## Frame 1 (CAN ID: 0x0C0)
* EngineRPM: Raw = 0x1027 (4135) -> Physical = 1033.75 rpm
* CoolantTemp: Raw = 0x96 (150) -> Physical = 35.0 °C
* ThrottlePos: Raw = 0x64 (100) -> Physical = 40.0 %
* EngineRunning: Raw = 0x01 (1) -> Physical = 1

## Frame 2 (CAN ID: 0x0C0)
* EngineRPM: Raw = 0x204E (8270) -> Physical = 2067.5 rpm
* CoolantTemp: Raw = 0xB4 (180) -> Physical = 50.0 °C
* ThrottlePos: Raw = 0xC8 (200) -> Physical = 80.0 %
* EngineRunning: Raw = 0x01 (1) -> Physical = 1

## Frame 3 (CAN ID: 0x1A0)
* SpeedFL: Raw = 0x07D0 (2000) -> Physical = 20.0 km/h
* SpeedFR: Raw = 0x07D0 (2000) -> Physical = 20.0 km/h
* SpeedRL: Raw = 0x07D0 (2000) -> Physical = 20.0 km/h
* SpeedRR: Raw = 0x07D0 (2000) -> Physical = 20.0 km/h

## Frame 4 (CAN ID: 0x320)
* DoorFL: Bit 0 = 1 -> open
* DoorFR: Bit 1 = 0 -> closed
* DoorRL: Bit 2 = 1 -> open
* DoorRR: Bit 3 = 0 -> closed
* Hazard: Bit 4 = 1 -> on
* IgnitionOn: Bit 8 = 1 -> on
* BatteryVolt: Raw = 0xBC (188) -> Physical = 18.8 V

## Frame 5 (CAN ID: 0x0C0)
* EngineRPM: Raw = 0x0000 (0) -> Physical = 0.0 rpm
* CoolantTemp: Raw = 0x30 (48) -> Physical = -16.0 °C
* ThrottlePos: Raw = 0x00 (0) -> Physical = 0.0 %
* EngineRunning: Raw = 0x00 (0) -> Physical = 0

---

# Exercise 2 — Target Deliverables

* Target A (EngineStatus 0x0C0): 0x0C0#00320000F0960001
* Target B (BCMStatus 0x320): 0x320#0A037E
*
