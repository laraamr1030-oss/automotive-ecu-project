
CAN Protocol Justification

​Why CAN (Controller Area Network)? CAN is chosen as the primary backbone for our powertrain and body control domains because it offers an optimal balance of cost, robustness, and speed (up to 1 Mbps). It uses a multi-master differential bus topology, ensuring high noise immunity which is critical for harsh automotive environments.

Trade-off Analysis vs. Alternatives:

vs. LIN (Local Interconnect Network): LIN is too slow (max 20 kbps) and is limited to simple sub-network devices (like window lifts or mirrors). It lacks the bandwidth required for real-time powertrain telemetry like engine RPM and BMS data.

​vs. FlexRay: While FlexRay offers higher speed and determinism, it is significantly more expensive and complex to implement, making it overkill for our standard multi-ECU project scope.

vs. Ethernet (Automotive Ethernet): Ethernet provides massive bandwidth for high-definition cameras and infotainment systems, but introduces unnecessary protocol overhead and hardware costs for standard ECU sensor-to-dashboard communication.


Bus-Load Calculation

Standard CAN 2.0 Frame Overhead: A standard CAN frame consists of an average of about 111 bits per message (including arbitration ID, control bits, data payload, CRC, and ACK slots).

Our Team's Message Set & Cycle Times:

Engine ECU (0x0C0): Transmitted every 10 ms (100 messages/sec).

Transmission ECU (0x0D0): Transmitted every 20 ms (50 messages/sec).

BCM (0x320): Transmitted every 100 ms (10 messages/sec).

BMS ECU (0x350 / Theme ECU): Transmitted every 50 ms (20 messages/sec).


Total Bitrate Consumption:

Engine: 100 \times 111 = 11,100 bits/sec

​Transmission: 50 \times 111 = 5,550 bits/sec

​BCM: 10 \times 111 = 1,110 bits/sec

​BMS: 20 \times 111 = 2,220 bits/sec

​Total Traffic: 19,980 bits/sec (~20 kbps)


Bus Load Formula & Result:


\text{Bus Load (\%)} = \left( \frac{\text{Total Traffic}}{\text{CAN Bus Speed (1,000,000 bps)}} \right) \times 100

vcan0 Setup Confirmation
​Interface Verification:

 The system utilizes Linux SocketCAN virtual interface (vcan0) to emulate physical hardware buses during development.


​Validation: 

Verified that all ECU processes successfully bind to vcan0, allowing concurrent broadcasting and monitoring via utilities like candump and cansniffer without packet loss.
