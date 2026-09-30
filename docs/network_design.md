# Phase 1: Network & Bus Design

## Why CAN
CAN is the backbone for the powertrain and body domains because it balances cost, robustness and speed
(up to 1 Mbps classic CAN, 500 kbps typical for powertrain). It is a multi-master differential bus with
arbitration by ID and built-in error detection, which suits a noisy vehicle.

| Alternative | Why not for this project |
| :--- | :--- |
| LIN (max 20 kbps) | Single-master, far too slow for RPM/SoC telemetry and UDS transfers; fine for mirrors/windows. |
| FlexRay (10 Mbps) | Deterministic but expensive and complex; overkill for a handful of ECUs at 10 Hz. |
| Automotive Ethernet (100 Mbps+) | Needed for cameras/infotainment, but switch/PHY cost and protocol overhead are unjustified here. |

Per the architecture diagram: Powertrain CAN runs at **500 kbps**; that is the rate used below.

## Bus-load calculation (real message set)

| CAN ID | Sender | Period | Msgs/s | DLC | Bits/frame |
| :--- | :--- | ---: | ---: | ---: | ---: |
| 0x0C0 | Engine ECU | 100 ms | 10 | 8 | 111 |
| 0x0C1 | Engine ECU (fault status) | 100 ms | 10 | 3 | 71 |
| 0x0D0 | Transmission ECU | 100 ms | 10 | 8 | 111 |
| 0x320 | BCM | 100 ms | 10 | 8 | 111 |
| 0x350 | BMS ECU | 100 ms | 10 | 8 | 111 |

Bits per standard frame (no stuffing) = 47 + 8 × DLC → DLC 8 = 111 bits, DLC 3 = 71 bits
(SOF 1 + ID 11 + RTR 1 + IDE 1 + r0 1 + DLC 4 + data + CRC 15 + CRC-delim 1 + ACK 2 + EOF 7 + IFS 3).

```
Traffic = 4 frames x 10/s x 111 bits + 1 frame x 10/s x 71 bits
        = 4440 + 710 = 5150 bit/s

Bus load = 5150 / 500 000 x 100 = 1.03 %     (0.52 % if the bus ran at 1 Mbps)
```
Worst case with maximum bit stuffing (135 bits for DLC 8, 85 bits for DLC 3):
4 × 10 × 135 + 10 × 85 = 6250 bit/s → **1.25 %**. UDS traffic is on demand only (a 29-byte freeze-frame answer =
1 First Frame + 4 Consecutive Frames + 1 Flow Control ≈ 670 bits, i.e. a 1.3 ms burst) and does not change the result.
Conclusion: the bus is very lightly loaded (well under the usual 30-40 % design limit), leaving room for growth.

## vcan0 setup (Session 6)
```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
ip link show vcan0            # must show "UP"
```
All ECU processes and the dashboard open a `PF_CAN / SOCK_RAW` socket bound to `vcan0`. Verified with
`candump vcan0` / `cansniffer vcan0` showing 0x0C0, 0x0C1, 0x0D0, 0x320 and 0x350 simultaneously
(capture: `docs/evidence/candump_all_ids.log`). Note `vcan0` has no bit rate; the 500 kbps figure is the
physical bus this virtual bus models.
