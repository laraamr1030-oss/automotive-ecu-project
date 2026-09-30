# Member 1 files that block everyone else (apply FIRST)
Copy over the repo (same paths):  cp -r member1_required_fixes/* <repo>/
- transmission_ecu.cpp/.h : the old ECU never sent 0x0D0 (main.cpp never opened the socket, it listened to the old 0x100). Now it listens to 0x0C0 and broadcasts speed+gear on 0x0D0.
- bms_ecu.cpp/.h : scripted 60 s cycle so pack over-temp (P0A7E) and pack under-voltage (P0AFA) appear and disappear.
git add -A && git commit -m "Transmission ECU broadcasts 0x0D0; BMS scripted fault scenario"
