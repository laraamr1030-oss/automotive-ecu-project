# Phase 2: CAN Signal Dictionary

| CAN ID | ECU Name | Signal Name | Start Bit | Bit Length | Scale | Offset | Unit |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **0x0C0** | Engine ECU | Engine Speed (RPM) | 0 | 16 | 0.25 | 0 | RPM |
| **0x0C0** | Engine ECU | Engine Temp | 16 | 8 | 1.0 | -40 | °C |
| **0x0D0** | Transmission | Vehicle Speed | 0 | 16 | 0.01 | 0 | km/h |
| **0x0D0** | Transmission | Current Gear | 16 | 8 | 1.0 | 0 | Gear |
| **0x320** | BCM | Door / Light Status | 0 | 8 | 1.0 | 0 | Bitfield |
| **0x250** | *Theme ECU* | *TBD by Theme* | 0 | 16 | 1.0 | 0 | TBD |
| **0x7E0** | UDS Tester | Diagnostic Request | 0 | 64 | - | - | Raw |
| **0x7E8** | UDS Response| Diagnostic Response| 0 | 64 | - | - | Raw |
