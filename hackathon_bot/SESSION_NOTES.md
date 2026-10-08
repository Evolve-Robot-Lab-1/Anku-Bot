# Hackathon bot session — 2026-10-04

## Arm servo retest — 2026-10-07

- User confirmed arm support, clear path, and actuator power. Mega `/dev/ttyACM0` reported `FOUR_MOTOR_PCA_ARM` and detected PCA9685 at `0x41`.
- Ran one `A` CH0 0°→60°→0° sweep, then one `B` CH1 0°→60°→0° sweep. Both reported `SERVO_START` and `SERVO_CYCLE_COMPLETE`. `STOP` was acknowledged after each and once more at the end. Physical movement confirmation remains pending, especially for CH0, which previously did not visibly sweep.
- At user's request, ran one `G` suction/pump cycle on CH6 valve and CH7 pump: phase 0 for 3,000 ms, phase 1 for 1,000 ms, phase 2 for 250 ms. Mega reported `GRIPPER_CYCLE_COMPLETE`; explicit `STOP` was acknowledged. Awaiting physical confirmation of suction and release, especially whether the pump stops and valve vents during release.
- Repeated the same `G` cycle once more at user's request. All three phases and `GRIPPER_CYCLE_COMPLETE` were reported; explicit `STOP` acknowledged. Physical release outcome still awaits user observation.
- User reported that release is incomplete and the object takes time to detach. Current firmware vents with CH6=2000 µs and pump neutral CH7=1500 µs for only 1 second, then returns CH6 to neutral for 250 ms before disabling PCA outputs. Proposed next diagnostic: confirm the pump actually stops and CH6 opens an atmospheric vent into the cup-side line during release. If venting works, test holding the vent open longer and remove the premature neutral phase. If it does not, inspect valve type and tubing; timing changes alone will not fix a blocked or incorrectly placed vent. A brief controlled air blow-off may help after the basic vent path is verified. No firmware or wiring changes were made for this release issue yet.

## Four-motor retest — 2026-10-07

- User confirmed wheels were raised and a motor-power stop was available. Pi Wi-Fi SSH at `192.168.1.8` connected; Mega 2560 R3 was on `/dev/ttyACM0` with `FOUR_MOTOR_PCA_ARM` firmware.
- Ran one `F` command: all four motors commanded at PWM 200 for 1 second. Mega reported `START,MASK,15,F,PWM200,1000MS` then `STOPPED,TIMEOUT`; an explicit `STOP` was sent afterward and acknowledged.
- Final stable encoder readings after coast-down: M1 1,961 A edges, M2 1,973, M3 1,874, M4 1,845. Every encoder observed both B levels at A transitions. User subsequently confirmed all four wheels physically spun. Automatic stop and an additional explicit `STOP` were acknowledged.

## Camera and LiDAR retest — 2026-10-07

- User deferred vacuum release troubleshooting and moved to camera/LiDAR checks.
- Logitech C270 is connected to Pi USB as `/dev/video0`. It emits about 30 black startup frames, then valid 640×480 images in both YUYV and MJPG modes. The warmed-up image showed the tabletop, screwdriver, and cables; disk and plate hole were not in view. The live preview was started on Pi Wi-Fi at `http://192.168.1.8:8000/` and returned HTTP 200 from this PC.
- Updated `tools/detect_circles.py` to wait up to 8 seconds for three usable camera frames instead of saving an initial black frame. Its synthetic circle self-test passed. Real disk/hole detection remains pending until both objects are in the camera view.
- D500 LiDAR is connected through CP2102 (`10c4:ea60`) at `/dev/ttyUSB0`. Four-second read-only serial check at 230400 baud: 78,210 bytes, 1,663 valid packets, 0 CRC failures, 19,951 plausible range points, all 36 ten-degree sectors, 80–6,192 mm, median 677 mm, median rotation 9.93 Hz. Raw scan healthy; no ROS scan publication was tested in this retest.

## PCA servo test firmware — 2026-10-07

- Updated the Mega sketch to `FOUR_MOTOR_PCA_ARM`; AVRDUDE verified all 12,846 bytes on `/dev/ttyACM0`.
- Startup found PCA9685 at `0x41`, configured 50 Hz, and disabled all PCA outputs. D7 is configured as active-low PCA OE. `A` plus newline starts the limited CH0 0°→60°→0° sweep; `STOP` disables PCA and motor outputs. Four-motor test commands remain available.
- User reported the initial 25 ms/degree sweep was slow. Updated it to 10 ms/degree (about 1.2 seconds for the full cycle), rebuilt, and verified the 12,846-byte flash upload. Sent `A` once; Mega reported `SERVO_START,CH0,0_TO_60_TO_0` and `SERVO_CYCLE_COMPLETE,CH0`. Sent `STOP` afterward. Physical motion of the faster sweep has not yet been confirmed visually.
- Latest request restored the CH0 step interval to 25 ms (about 3 seconds per full sweep). Rebuilt/uploaded and verified 12,846 bytes. Ran `A` once; Mega reported both `SERVO_START,CH0,0_TO_60_TO_0` and `SERVO_CYCLE_COMPLETE,CH0`. Sent `STOP` afterward. This slow setting is the current Mega firmware.
- User reported CH0 was not physically sweeping. Live Mega scan still found PCA9685 at `0x41`. User reported 5 V servo V+ and OE initially unconnected; after connecting OE to Mega D7, a repeat slow `A` cycle again reported `SERVO_START` and `SERVO_CYCLE_COMPLETE`, followed by `STOP`. Awaiting user's physical movement observation; serial completion alone does not prove servo motion.
- User connected servo 2 to PCA CH1. Added `B` plus newline for an isolated 25 ms/degree CH1 0°→60°→0° sweep; `A` still tests CH0. Rebuilt/uploaded and verified 12,940 bytes. Running `B` found PCA `0x41` and reported `SERVO_START,CH1,0_TO_60_TO_0` then `SERVO_CYCLE_COMPLETE,CH1`; sent `STOP`. User confirmed servo 2 physically moved.
- User connected prior valve/solenoid to PCA CH6 and vacuum pump control to CH7. Added `G` to the Mega sketch with the previously tested sequence: CH6=1500/CH7=2000 µs for 1 second, CH6=2000/CH7=1500 µs for 0.5 second, both 1500 µs for 0.25 second, then outputs off. Rebuilt/uploaded and verified 13,534 bytes. The Mega found PCA `0x41` and reported gripper phases 0, 1, 2, and `GRIPPER_CYCLE_COMPLETE`; sent `STOP`. Awaiting physical suction/release observation.
- Latest `G` timing is now CH6=1500/CH7=2000 µs for 3 seconds, CH6=2000/CH7=1500 µs for 1 second, both 1500 µs for 0.25 second, then disabled. Rebuilt/uploaded and verified 13,576 bytes. Ran `G` once; Mega reported phases `0,MS,3000`, `1,MS,1000`, `2,MS,250`, and `GRIPPER_CYCLE_COMPLETE`; sent `STOP`. Physical suction/release outcome has not been reported yet.
- User reported release was slow and the cup seemed to keep sucking. Repeated the same `G` cycle without changing timings; Mega again reported PCA `0x41`, all three phases, and completion, followed by `STOP`. Need physical observation of whether CH7 pump stops during release and whether CH6 valve actually vents air. The earlier Uno bench notes observed CH7 1500 µs as pump off and CH6 2000 µs as valve click/release with its then-current tubing.

## Mega PCA9685 connection — 2026-10-07

- User moved M4 encoder A from D20 to D17 so Mega I2C can use SDA=D20/SCL=D21. D17 is not a Mega external-interrupt pin; the current test firmware polls M4 A in `loop()`.
- Uploaded `arduino/four_motor_test/four_motor_test.ino` as `FOUR_MOTOR_PCA_SCAN`; AVRDUDE verified all 8,796 bytes on the Mega at Pi `/dev/ttyACM0`.
- Startup I2C scan found `0x41` (expected PCA9685 address) and `0x70` (PCA9685 all-call address). PCA9685 detection confirmed. No servo output or motor pulse was commanded during this check.
- All prior motor test commands remain (`F`/`R`, `M1F`..`M4R`, `STOP`, `STATUS`, `RST`). M1–M3 pins are unchanged. M4 direction D28/D29, PWM D8, encoder B D39, encoder A now D17 (polled).
- External actuator power was off during upload. Keep it off until the arm-control firmware is integrated and the wiring/output-enable behavior is checked.

## Motor diagnostic update — 2026-10-07

- Initial Mega I2C scan firmware: `arduino/four_motor_test/four_motor_test.ino`, FOUR_MOTOR_PCA_SCAN, 8,796 bytes verified. Superseded by FOUR_MOTOR_PCA_ARM above.
- M3: direction D26/D27, PWM D9, encoder A D3/B D37. Current M4: direction D28/D29, PWM D8, encoder A D17 (polled)/B D39. D20/D21 now serve Mega SDA/SCL for the PCA9685.
- Four-motor `F` command pulses PWM 200 for 1 second and stops all. Last all-motor test reported STOPPED. Final encoder readings: M1 0 A edges; M2 1,987 A edges, B HIGH at all A edges; M3 1,912 A edges, B HIGH 955/LOW 957, signed 1,910; M4 1,893 A edges, B HIGH 946/LOW 947, signed 1,893. Physical wheel movement was not visually confirmed during this test. M1 loss of A edges in this all-motor run is new compared with its earlier individual test and needs inspection.
- Follow-up 2026-10-07: M4-only `M4F` pulse ran 1 second, auto-stopped, 1,932 A edges, B balanced HIGH/LOW. All-motor `F` pulse also auto-stopped; M1 0 A edges (B final LOW), M2 2,020 A edges with B always HIGH, M3 1,923 A edges with B balanced, M4 1,904 A edges with B balanced. M1/M2 results remain abnormal; physical rotations not directly observed.
- Latest user-requested rerun: all four motors received a one-second `F` pulse at PWM 200; Mega then reported stopped. All encoders produced A edges and both B levels during A transitions: M1 1,955 edges, M2 2,035, M3 1,931, M4 1,933. User confirmed "works". Treat this as the latest successful bench result; earlier intermittent M1/M2 observations remain history, not current status.
- Active pin map: M1 IN3=D24, IN4=D25, ENB/PWM=D11, encoder A=D19/B=D18; M2 IN1=D22, IN2=D23, ENA/PWM=D10, encoder A=D2/B=D35; M3 IN1=D26, IN2=D27, ENA/PWM=D9, encoder A=D3/B=D37; M4 IN3=D28, IN4=D29, ENB/PWM=D8, encoder A=D17/B=D39. Shared grounds required. D20/D21 are Mega SDA/SCL for PCA9685.
- Current Mega firmware: `FOUR_MOTOR_PCA_ARM` (see latest section above). The four-motor commands `F`/`R`, `M1F`..`M4R`, `STOP`, `STATUS`, and `RST` remain. Motor pulse tests do not establish direction alignment or sustained-load behavior.

- Current connected board on Pi is Arduino Mega 2560 R3, USB ID `2341:0042`, `/dev/ttyACM0` (previous CH340 board was disconnected).
- Current verified uploaded source: `arduino/two_motor_test/two_motor_test.ino`, TWO_MOTOR_V2, 6,624 bytes verified by avrdude.
- User-confirmed wiring: M1 direction D24/D25, PWM D11, encoder A D19 / B D18. M2 direction D22/D23, PWM D10, encoder A D2 / B D35. This differs from the original four-wheel reference PWM map.
- F/R pulses both motors; M1F/M1R/M2F/M2R pulse one motor, PWM 200, 1 second automatic stop. STOP disables both. Startup disables PWM D8–D12 and direction D22–D29. No motion starts automatically.
- A CHANGE interrupts count edges and sample B at every A edge for each motor; reports separate signed raw ticks, total A edges, and B HIGH/LOW observation counts. No encoder revolution calibration or closed-loop control is implemented.
- Last both-motor pulse: M1 2,032 A edges, B HIGH at all edges; M2 1,703 A edges, B HIGH at all edges. Both B LOW observation counts zero. STOP and STATUS verified stopped.
- Earlier physical swap for M1: original A produced changing levels on D18, while original B produced zero A edges on D19; issue followed original B lead/output. Exact motor encoder pinout, supply voltage, and B integrity remain unverified. Do not infer a confirmed failed encoder from these readings.

## Wi-Fi SSH update — 2026-10-05

- Pi 5 connected to `EVOLVE ROBOT LAB 2.4` on `wlan0`, current address `192.168.1.8`.
- Verified key-based SSH from this PC: `ssh sanjeev@192.168.1.8`; return route uses `wlan0`.
- Saved Wi-Fi profile UUID `501bef58-5b15-467a-bc81-954554677093` has autoconnect enabled. Reboot behavior not yet tested; DHCP address may change.
- Ethernet remains available at `192.168.50.62`. No actuator commands issued.

## Pi access

- Raspberry Pi 5 Model B Rev 1.1, Ubuntu 24.04.5 LTS, ROS 2 Jazzy.
- Hostname `sanjeev-desktop`, user `sanjeev`.
- Connect from PC: `ssh sanjeev@192.168.50.62`.
- PC Ethernet `enp4s0`: `192.168.50.1/24`; Pi uses DHCP, so its address can change.
- Pi Ethernet MAC `88:a2:9e:a0:58:3e`.
- PC profile `med-robot-pi-direct` shares internet; uplink `wlxa4e61559667d`.
- SSH was missing. Prepared the SD card with this PC's public key and a one-time systemd installation task. SSH installation completed and key login was verified.
- Pi user belongs to `dialout` and was added to `video`.

Docker's FORWARD DROP policy blocked Pi downloads. User added these temporary PC rules; they may need restoring after a reboot or firewall reload:

```bash
sudo iptables -I DOCKER-USER 1 -i enp4s0 -o wlxa4e61559667d -s 192.168.50.0/24 -j ACCEPT
sudo iptables -I DOCKER-USER 1 -i wlxa4e61559667d -o enp4s0 -d 192.168.50.0/24 -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
```

## USB camera — passed

- Logitech C270, USB ID `046d:0825`, `/dev/video0`.
- Stable capture device: `/dev/v4l/by-id/usb-046d_C270_HD_WEBCAM_31DEC370-video-index0`.
- OpenCV 4.6.0 installed. Captured frames at 640×480; initial image was black, subsequent live images were clear.
- Live feed verified by capture health and user: `http://192.168.50.62:8000/`.
- Server: `/home/sanjeev/hackathon_bot/tools/live_usb_camera.py`; uses adjacent `live_camera.py` for HTTP serving.
- Log: `/home/sanjeev/hackathon_bot/usb-camera.log`. Started with nohup, not configured for reboot.

## D500 LiDAR — raw serial passed

- Same LDROBOT D500 model as medical robot.
- CP2102 USB ID `10c4:ea60`, `/dev/ttyUSB0`.
- Stable path: `/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0`.
- 230400 baud; streams without commands. Checked 47-byte `54 2c` packets and CRC-8 polynomial `0x4d`, following LDROBOT SDK.
- Four-second capture: 78,272 bytes, 1,664 valid packets, zero CRC failures; 19,968 points, 18,860 within the diagnostic 30–12,000 mm range.
- Returns covered all 36 ten-degree sectors; median rotation 9.93 Hz; plausible distances 30–5,905 mm, median 456 mm.
- Script: `tools/check_d500.py`, also copied to Pi under `/home/sanjeev/hackathon_bot/tools/`.
- `rplidar_ros` is installed but is not a D500 driver. D500 ROS scan publication/mapping was not configured or tested in this session.

## Arduino arm and gripper — bench tests passed

- FTDI USB ID `0403:6001`, `/dev/ttyUSB1`.
- Stable path: `/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A5069RR4-if00-port0`.
- ATmega328P signature `0x1e950f`; compiled/uploaded as Arduino Uno at 115200 baud.
- PCA9685 address `0x41`, SDA A4, SCL A5, OE D8; same wiring as prior medical bench tests.
- Reported supply 5 V; CH0/CH1 servos, CH6 solenoid, CH7 vacuum pump.
- Original loaded firmware was the medical gripper test. Backed up before replacement:
  - Pi `/home/sanjeev/hackathon_bot/arduino_backup/before-arm-test.hex`
  - Pi `/home/sanjeev/hackathon_bot/arduino_backup/before-arm-test-eeprom.hex`
- Installed `avrdude` on Pi.
- Current source: `arduino/arm_gripper_test/arm_gripper_test.ino`.
- Current compiled image on Pi: `/home/sanjeev/hackathon_bot/arm_gripper_test.ino.hex`; final upload wrote and verified 8,874 bytes.
- All PCA outputs disabled at startup. Serial commands:
  - `G`: suction 1 second (CH6 1500 µs, CH7 2000 µs), release 0.5 seconds (CH6 2000 µs, CH7 1500 µs), neutral 0.25 seconds, then disable.
  - `A`: CH0 single nominal 0→60→0 sweep.
  - `B`: CH1 single nominal 0→60→0 sweep.
  - `C`: CH0 and CH1 together, single nominal 0→60→0 sweep.
  - `S`: stop and disable all PCA outputs.
  - `1`–`6`: retained individual gripper pulse tests; see medical gripper README before use.
- Servo mapping matches prior tested sketch: nominal 0–180° maps to 1000–2000 µs, 25 ms per degree. Physical joint angles are not calibrated.
- User confirmed gripper action, each servo separately, and both together worked.
- Continuous host-driven sweeps ran using `tools/repeat_arm_sweep.py`; eight full cycles logged, then stopped during the ninth at user's request.
- Stop control: create `/home/sanjeev/hackathon_bot/STOP_ARM_SWEEP` on Pi.
- At end: repeating process absent; separate serial stop explicitly acknowledged `OUTPUT DISABLED`. Stop file remains to prevent accidental restart. Removing control pulses does not cut V+ power or guarantee the arm holds its pose.
- Sweep log: `/home/sanjeev/hackathon_bot/arm-sweep.log`.

## Resume

1. Connect by SSH; confirm device paths and supply/wiring before hardware commands.
2. Camera server may still be running; after reboot it must be started again.
3. Arm sweep is stopped. Do not restart it automatically; only resume on explicit user instruction with arm supported and path clear.
4. Wheel/base Arduino serial and motor tests remain pending. The connected Uno is the arm controller, not the four-wheel base controller.
5. Four-wheel source is still unvalidated on this hackathon robot; do not infer its wiring from the medical two-motor controller.

Older copied Ethernet scripts still assume user `ubuntu` and/or Pi `192.168.50.2`; use the verified SSH command above instead.

## Session update — navigation deferred

User discussed and approved an indoor mapping/waypoint plan, then deferred implementation because wheel motor tests are not done. The initial inspection tool call was aborted; no navigation implementation or wheel firmware upload was completed.

Agreed future scope: ROS 2 Jazzy, D500 scans, calibrated wheel odometry, SLAM Toolbox mapping, AMCL/Nav2 localization and navigation, browser map/route controls. Although the robot has mecanum wheels, first version uses forward-and-turn driving only. Visit waypoints in order, pause 3 seconds, keep arm disabled, and stop/report an unreachable goal. Initial planned limits: 0.15 m/s and 0.3 rad/s. These are planned defaults, not tested hardware limits.

Next action: connect the separate wheel-controller Mega to the Pi with motor power off, identify its stable USB path and active firmware, and back up firmware before replacement. Verify physical wiring, then test each wheel and encoder individually with wheels raised and a physical power stop accessible. The arm Uno remains a separate controller. Do not start autonomous driving or assume the copied four-wheel firmware matches the wiring.

Arm sweep remains stopped; last verified Arduino state was OUTPUT DISABLED. No new arm or motor commands were issued during this update.

## Session update — wireless gamepad input checks

- User identified controller as HAMMOK Zeta 2 wireless gamepad. USB receiver on Pi enumerates as `shanwan Android GamePad`, VID:PID `2563:0526`.
- Input path `/dev/input/js0`; stable alias `/dev/input/by-id/usb-shanwan_Android_GamePad-joystick`; event device was `/dev/input/event5`.
- User initially powered/charged controller by cable from PC, then unplugged it. Wireless test on Pi received 723 live events, confirming stick/button input reception.
- The left directional control tested is a D-pad, not an analog stick.
- All mapping tests below were read-only. No gamepad-triggered arm or wheel commands were sent. Proposed joystick-to-arm bridge was not implemented or started.

| Physical control | Verified Linux joystick input |
| --- | --- |
| A | Button 0, press 1 / release 0 |
| B | Button 1, press 1 / release 0 |
| X | Button 3, press 1 / release 0 |
| Y | Button 4, press 1 / release 0 |
| D-pad Up | Axis 7, -32767; release 0 |
| D-pad Down | Axis 7, +32767; release 0 |
| D-pad Left | Axis 6, -32767; release 0 |
| D-pad Right | Axis 6, +32767; release 0 |
| R1 | Button 7, press 1 / release 0 |
| R2 | Button 9, press 1 / release 0; axis 4 released -32767 to fully pressed +32767 |
| L1 | Button 6, press 1 / release 0 |
| L2 | NOT VERIFIED: initial device path missing; retry opened successfully but captured no live events |

Receiver intermittently disappeared from the Pi's input paths and re-enumerated during A, R2, and L2 checks. Cause not established. Investigate USB connection/power and receiver stability before using it for actuator control. Wireless loss behavior while receiver remains attached has not been tested; do not assume Linux device presence guarantees a live radio connection.

Suggested future arm trial: A triggers the previously validated single CH0 sweep, X triggers CH1, B stops. This remains a proposal, not active control. First complete L2 mapping, confirm controller stability and release behavior, and implement bounded motion with a verified stop path. Arm's last explicitly checked serial state during this gamepad session was OUTPUT DISABLED.

Next pending checks: L2 input mapping, then gamepad disconnect behavior and arm-control bridge if requested. Wheel motor validation and autonomous mapping/navigation remain deferred.

## Bot 2 Pi 4B and HAT check — 2026-10-07

- New Raspberry Pi 4 Model B Rev 1.5, Ubuntu 24.04.5 LTS, hostname `hackathon-bot2`, user `sanjeev`.
- Wi-Fi SSH verified from this PC at `hackathon-bot2.local` (`192.168.1.10` at the time of check). SSH service was active. The Pi is separate from Bot 1 at `192.168.1.8`.
- I²C bus `/dev/i2c-1` is enabled. Added `sanjeev` to `dialout`; a fresh SSH session confirmed access.
- Read-only I²C probe at `0x5f` succeeded: register `0x00` (MODE1) = `0x11`, `0x01` (MODE2) = `0x04`, and `0xfe` (PRE_SCALE) = `0x1e`. These readings are consistent with the PCA9685 controller expected on the Adeept Robot HAT.
- No motor commands or output-setting writes were sent. HAT revision, wiring, motor power, and physical motor operation remain unverified.

## Bot 1 individual motor direction test — 2026-10-07

- User confirmed all wheels raised, area clear, and a motor power stop available before actuation. Pi Wi-Fi SSH `192.168.1.8`; Mega 2560 on `/dev/ttyACM0` reported `FOUR_MOTOR_PCA_ARM` and PCA9685 at `0x41`.
- Tested `M1F`, `M2F`, `M3F`, `M4F`, then `M1R` through `M4R` one at a time. Each command used firmware's PWM 200 for 1 second, reported `STOPPED,TIMEOUT`, and was followed by an explicit `STOP` acknowledged as `STOPPED`. M1R and M4R were repeated at the user's request.
- User's physical direction observations: `F` rotated M1, M2, M3 in physical reverse and M4 in physical forward; `R` rotated M1, M2, M3 in physical forward and M4 in physical reverse. Wheel positions were not identified, so this is motor-index direction only.
- A-edge counts during the first pulses: M1F 1,637; M2F 1,645; M3F 1,579; M4F 1,573. Reverse: M1R 1,576 (repeat 1,574); M2R 1,672; M3R 1,605; M4R 1,557 (repeat 1,558). In every single-motor test, other motors' A-edge counts stayed zero.
- M1, M2, M4 B levels at A edges were balanced in both directions. M3F had B high at all 1,579 A edges; M3R had B high at 1,383 and low at 222 A edges, giving only -445 signed ticks despite 1,605 A edges. This intermittent M3 B signal needs inspection before using its signed encoder ticks for odometry or closed-loop motion.
- No wiring or firmware changes were made. Do not assume firmware `F` means physical forward for the whole robot until direction mapping is corrected and physical wheel positions are recorded.

## Bot 1 M3 encoder follow-up — 2026-10-07

- Repeated `M3F` at PWM 200 for 1 second with automatic timeout and explicit STOP. M3 A recorded 1,582 edges; B was high at all 1,582 A edges, giving signed ticks 0. Other motor A-edge counts stayed zero.
- With motor power off, user hand-turned M3 during a 15-second encoder-only read. After reset, M3 reached 763 A edges, B high at 379 A edges and low at 384, with signed ticks -753. Both raw A and B levels changed during the test. No motion command was sent.
- M3 quadrature works during slow manual rotation but becomes unreliable during powered operation. Root cause is not yet established; inspect the M3 encoder B lead to Mega D37, encoder power and shared ground, connector security, and routing near motor power wires before relying on M3 signed ticks. Do not assume the B sensor itself is failed from these results.

## Bot 1 M3 encoder retest after reconnection — 2026-10-07

- User reconnected the M3 encoder connection. A one-second `M3F` pulse at PWM 200 reported `STOPPED,TIMEOUT`; explicit `STOP` was acknowledged. M3 recorded 1,596 A edges, B high at 798 and low at 798 A edges, with signed ticks +1,596. This is a balanced quadrature result under powered operation and resolves the previously observed B-stuck-high symptom in this bench retest. M1 logged one stray A edge; M2 and M4 logged zero. Longer-run reliability remains untested.

## Bot 1 four-motor bench pulse — 2026-10-07

- Ran one `F` command with all four motors (mask 15), PWM 200 for 1 second. Firmware reported `STOPPED,TIMEOUT`, and explicit `STOP` was acknowledged. Encoder A edges: M1 1,587; M2 1,616; M3 1,591; M4 1,564. B levels at A edges were balanced for all four: M1 794/793, M2 808/808, M3 796/795, M4 782/782. This confirms the M3 B signal remained balanced under simultaneous motor operation in this pulse. User visual confirmation was pending when logged. Firmware `F` maps to physical reverse for M1–M3 and forward for M4 from earlier individual tests, so this pulse does not establish chassis forward travel.

## Bot 1 smooth arm profile trial — 2026-10-07

- Replaced 25 ms/degree linear 0°→60°→0° sweep on Mega PCA CH0/CH1 with a cubic ease-in/ease-out profile, 1.5 seconds per leg, 20 ms updates. Compiled (13,710 flash bytes), uploaded to Mega over Pi, and AVRDUDE verified all 13,710 bytes. Prior source saved as `arduino/four_motor_test/four_motor_test.ino.pre_smooth.bak`; current source matches flashed firmware. Motor and gripper commands were otherwise unchanged.
- Ran one CH0 `A` cycle: firmware reported start, completion, and explicit STOP; user said physical movement was smooth.
- Ran one CH1 `B` cycle: firmware reported start, completion, and explicit STOP; user said outward motion toward 60° was abrupt. CH1 smoothness remains unresolved. The cubic curve has higher peak speed than the prior constant-speed sweep at the same 3-second round trip, so a slower CH1 profile may be needed; an endpoint restriction is also possible. No further CH1 motion was commanded pending where the abruptness occurs.

- CH1 `B` rerun after the smooth-profile trial: Mega again reported `SERVO_START,CH1`, `SERVO_CYCLE_COMPLETE,CH1`, and explicit STOP, but user saw no physical movement. Further arm commands paused pending inspection of CH1 servo power, connector, and mechanical linkage. Serial completion does not confirm physical motion.

- CH1 was run once more with `B`: firmware reported start, completion, and STOP; user again saw no movement. CH0 comparison `A` then reported start/completion/STOP and user confirmed physical movement. Shared PCA I²C/OE and at least CH0 servo power were functional in this comparison. Focus CH1 diagnosis on its three-wire plug orientation/seating, channel wiring/output, servo, and mechanical linkage; do not infer the servo itself is failed yet. No further CH1 sweeps pending physical inspection.

- Latest CH1 observation: user identified CH1 as shoulder up/down and CH0 as shoulder forward/back. On the latest CH1 sweep, the shoulder lifted quickly, then lowered slightly after a while. This suggests CH1 peak speed is too high and/or output-off sag, but the timing of the drop and desired endpoint are being clarified.

## Bot 1 CH1 slower lift-and-hold trial — 2026-10-07

- Changed CH1 `B` to a one-way eased 0°→60° lift over 2.5 seconds that continues PWM output at 60°; added `C` for eased 60°→0° lower over 2.5 seconds. `STOP` disables all outputs. CH0 `A` retains its prior sweep. Motor/gripper commands are blocked while CH1 is moving or holding to avoid an implicit arm release. AVRDUDE verified 14,596 flashed bytes. Prior smooth cycle source saved as `four_motor_test.ino.smooth_cycle.bak`; current source matches flashed firmware.
- One serial session ran `B`, confirmed `SERVO_HOLDING,CH1,60`, held four seconds, ran `C`, confirmed `SERVO_HOLDING,CH1,0`, then explicit STOP. User reported no physical movement. This means the observed CH1 failure remains unresolved; no more CH1 motion commands should be repeated until connector/servo/channel isolation.

## CH2 sweep code verification — 2026-10-07

- The CH2 sweep source compiles for the Mega and uses a cubic ease-in/ease-out over a 2.5-second leg. The test command `D` is a 0°→10°→0° sweep on PCA CH2; `B`/`C` are separate lift/hold and lower commands. The CH0 command `A` remains 0°→60°→0°.
- User reported the 30° and then 10° CH2 sweeps raised the shoulder too far. Reduced candidate limit to 3° and compiled it successfully, but this 3° candidate was not flashed or physically tested. The Mega remains on the verified 10° CH2 firmware.
- Synced the project sketch to the 10° firmware currently flashed; preceding source saved as `four_motor_test.ino.pre_10_degree.bak`. No motion was commanded during code review.

- User observed the 0°–3° CH2 sweep moving the shoulder upward too quickly, then turned actuator power off. The profile was programmed 0°→3°→0° over 2.5 seconds per leg, using 1000→about 1017→1000 µs; the large/fast initial rise may be a jump from the servo’s unknown physical position to the assumed 1000 µs baseline. This is not yet confirmed. Do not repeat sweeps until a safe starting pulse is calibrated with the arm supported.

## Bot 1 CH2 too-far/fast resolve — 2026-10-07

- Symptom confirmed by user: CH2 moves very up and stays, even on 1-count (5 us) D test. Mega reported `SERVO_START,CH2,0_TO_1_TO_0,995_TO_1000_TO_START_US`, `SERVO_CYCLE_COMPLETE,CH2`, `SERVO_HOLDING,CH2,995`. 5 us cannot cause large motion by itself; root cause is initial capture transient from unknown slack position to assumed 995 us baseline on first OE enable, plus CH0/CH1 held disabled during CH2 tests. Serial completion does not prove commanded sweep caused the motion.
- Fix flashed: `arduino/four_motor_test/four_motor_test.ino` now keeps 995/1000 us 1-count limits and adds `E` (-5 us) / `T` (+5 us) relative jog from active hold with 990–1100 us clamps, 2.5 s eased legs, no output disable. `B`/`C` absolute lift/hold remain. Prior source saved as `four_motor_test.ino.pre_jog.bak`.
- Build: `arduino-cli compile --fqbn arduino:avr:mega:cpu=atmega2560`, 15,412 bytes. Copied to Pi `/tmp/bot1_ch2_jog.hex`; `avrdude -p atmega2560 -c wiring -P /dev/ttyACM0 -b 115200` verified all 15,412 bytes.
- Post-flash read-only check: PCA9685 `0x41` FOUND, `READY,FOUR_MOTOR_PCA_ARM`, help lists `E/T jog`, `STATUS` = `STOPPED`, explicit `STOP` acknowledged. No motion commanded after flash; outputs disabled, arm must stay supported until re-capture.
- Next: with arm supported, expect one snap when first re-enabling to 995 hold. After that use only `E`/`T` single steps; do not send `STOP` (drops hold) or open serial unnecessarily (resets Mega and loses hold position).

## Bot 1 CH2 uint8_t truncation bug — 2026-10-07 (critical)

- Found live: `C` reported `SERVO_START,CH2,995_TO_227` / `SERVO_HOLDING,CH2,227` instead of 1000→995. Cause: `startShoulder(uint8_t target)` truncated 995→227 (995 mod 256) and 1000→232. B/C drove CH2 to ~230 us extreme-low instead of ~1000 us. This explains far/up slam on B/C; D sweep was unaffected (uses uint16_t pulses directly).
- Immediate action: sent `STOP`, Mega acknowledged `STOPPED`, outputs released to stop 227 us stall. Arm stayed supported.
- Fix: signature to `startShoulder(uint16_t target)`. Rebuilt 15,406 bytes, flashed `/tmp/bot1_ch2_jog2.hex`, AVRDUDE verified. Post-check PCA `0x41` FOUND, help lists E/T jog, `STATUS STOPPED`. No motion commanded after fix. Source saved as `four_motor_test.ino.pre_u8fix.bak`.
- Caution: E/T jog requires active hold; first B/C capture after reset still snaps from slack to tracked pulse. Keep arm supported; use single small steps only.

## Bot 1 clean CH0 sweep (servo 2) — 2026-10-07

- User clarified servo 2 is on PCA CH0; prior CH2 driving hit an empty channel. Wrote clean `arduino/ch0_sweep/ch0_sweep.ino`: CH0-only 1000→1083→1000 us (~15°), 2.5 s/leg cubic ease, holds start pulse after, motors forced off. `A` sweep, `STOP` release, `STATUS` state.
- Built 9,558 bytes, flashed `/tmp/ch0_sweep.hex`, AVRDUDE verified. Mega `READY,CH0_SWEEP_SERVO2`, PCA `0x41` found.
- Ran `A` once: `SERVO_START`, `SERVO_CYCLE_COMPLETE`, `SERVO_HOLDING,CH0,1000`. Left holding; no STOP sent. 4-motor firmware replaced on Mega; reflash `four_motor_test` to restore wheel/encoder tests.

## Bot 1 CH0 reversed (servo 2) — 2026-10-07

- User moved servo 2 to PCA CH0. Clean `arduino/ch0_sweep/ch0_sweep.ino` (9558 B) swept 1000→1083→1000 but arm rose; `Z` to 1000 still read high. Root: horn clocking put 990–1100 range in up region.
- Reversed electrically: zero=2000, sweep end 1917, jog clamps 1800–2100, E/T ±5 us. 2000 verified down; user walked up to 1920 via E steps, then back to 2000. CH0 mapping: lower pulse = up, higher = down.
- Mega currently `READY,CH0_SWEEP_SERVO2`, holding CH0 2000. 4-motor firmware not on Mega; reflash `four_motor_test` (with uint16_t shoulder fix + E/T jog) to restore wheels.

## Bot 1 CH0 travel limits found — 2026-10-07

- Servo 2 on CH0, reversed zero=2000 (down). Walked up: 1920 held, 1900 held, 1880 stalled (tried then fell back = mechanical stop). Backed off to 1920 hold. Working range: 2000 (down) to ~1920/1900 (up); do not command below ~1900.

## Bot 1 CH0 angle map + zero return — 2026-10-07

- Nominal shaft scale 5.56 us/deg from down-zero 2000: 1920=+14.4° up (held), 1880=up stop (stall/fallback), 2080=-14.4° past-zero (held, still free). Span ~29° shaft-nominal; arm-joint ratio uncalibrated.
- Returned to 2000 hold on user request. Mega `READY,CH0_SWEEP_SERVO2`, holding CH0 2000. CH0-only firmware still on Mega.

## Bot 1 servo 2 free-range verify — 2026-10-07

- Horn off (unloaded), CH0-only fw with N0/N90/N180 + E/T jog (10,184 B). Verified N0=1000, N60~1335, N90=1500, N180=2000 all hold. Servo healthy full 0–180 electrically.
- Attached limits unchanged: ~1920 up-stop side, 2000 down-zero; reattach horn at 90°/mid-travel to center attached range. Mega holding CH0 2000.

## Bot 1 free-servo overdrive probe — 2026-10-07

- User insisted servo is 360 and asked 240 (refused: outside 0–180/1000–2000), then 190. Widened clamps to 900–2100 (10,184 B), stepped free servo to 2055 (~190° nominal): held, no stall reported.
- Returned to 2000 on request. Attached safe window unchanged (1900–2080); 240 not attempted.

## Bot 1 servo 2 wide-range result — 2026-10-07

- Free servo (horn off) holds 2220 us (~220° nominal) smoothly, no stall. Range exceeds standard 180°; likely 270°-class servo. User's push past 180 was correct. Clamps now 900–2250. Attached arm limits (linkage stops ~1880 up) are separate from servo capability.
- Mega holding CH0 2220.

## Bot 1 window sweep 200↔220 — 2026-10-07

- User set base=220 (2220 us), up=200 (2110 us). Added `W`: 2110→2220→2110, 2.5 s/leg eased, holds 2110. Flashed 10,506 B, verified live.
- Ran `W` once: start, cycle complete, holding 2110. Free servo (horn off).

## Servo points doc — 2026-10-07

- Saved working points (200 UP holding 2110, 220 FLAT 2220), angle map, firmware, commands, full source, history in `CH0_SERVO_POINTS.md`.

## Saved servo state — 2026-10-07

- `CH0_SERVO_POINTS.md` refreshed: dual poses Q/R/P, V slow sweep, full current source (11,656 B), CH1 base 60, CH0 window 200–220. Mega holding CH0 2220 + CH1 1335.

## Detailed servo doc refresh — 2026-10-07

- `CH0_SERVO_POINTS.md` expanded to full detail: dual hardware map, current dual hold
  (2220+1335), per-channel tables, complete command table (Q/R/P/V/W/A/B/Z/X/N/M/E/T/Y/U),
  build/flash/drive procedures, 6-point troubleshooting, detailed 7-step history.
  Embedded source refreshed to the 11,656 B dual build.

## Camera retest — 2026-10-08

- C270 on `/dev/video0` (verified via v4l2). Capture: black startup frames, valid 640×480 by frame 11 (mean 33.2), saved `/tmp/cam_test.jpg` on Pi.
- Preview server was down (stale default host 192.168.50.62 → bind fail). Restarted with `--host 0.0.0.0 --port 8000`; live at `http://192.168.1.8:8000/` (HTTP 200).

## Vision disk detection — 2026-10-08

- Camera repositioned; disk fully in view on patterned cloth (plate hole not in view).
- `tools/detect_circles.py` defaults found nothing (disk r≈168 exceeds default max).
  With `--min-radius 100 --max-radius 260`: one candidate x=245.4 y=298.2 r=168.4,
  annotated rim fit confirmed visually. Use these radii on the Pi for disk-class frames.

## Vision plate detection — 2026-10-08

- Metal plate over disk: default threshold gave 4 clutter circles (reflections).
  `--min-radius 100 --max-radius 260 --threshold 60` gives single rim fit
  x=245.4 y=304.2 r=180.5, confirmed on annotated frame.
- Saved Pi recipe: `python3 tools/detect_circles.py --min-radius 100 --max-radius 260 --threshold 60 --output vision/latest` (copy script to Pi first; Pi tools/ lacks it).

## CH11 suction-arm servo — 2026-10-08

- User: suction-cup arm servo is on PCA CH11. Extended firmware to 16-channel holds
  (12,026 B): `D` CH11 window sweep, `H` zero (2000), `H0/H90/H180`, `J/K` jog ±5.
  Same 900–2250 clamps. Verified live, idle `HOLDING,CH0,0,CH1,0,CH11,0`.
- Wheels still off (servo-only build). Mecanum driving needs `four_motor_test` reflash.

## Carry-all pose — 2026-10-08

- Serial-open resets kept dropping one channel. Added `C` = CH0 2220 + CH1 1335 +
  CH11 1000 in one command (12,200 B). Verified `POSE,CARRY,...` live.

## Rest pose + G setter — 2026-10-08

- Rest defined: CH0 210 (2165) + CH1 60 (1335) + CH11 60 (1335). New `F` one-shot.
- Generic setter `G<ch>,<us>` (e.g. `G11,1335`, keeps other holds). Banner shortened.
  Firmware 14,3xx B, verified, `F` live: POSE,REST triple held.

## Pick-ready — 2026-10-08

- CH11 at 0 (1000 us) = pick-ready position. Rest triple otherwise held
  (CH0 2165, CH1 1335).

## First pick cycle — 2026-10-08

- Sequence: C pick (220+60+pick-ready 0) → O suction on (gripped yes) → 210 carry →
  back to 220 → release L later → F rest (2165+1335+1335).
- Notes: flat placement needs press optimization (past-220 or dwell TBD).
  Suction resets clear on serial-open; re-send O after any reconnect.

## Base firmware back for mecanum — 2026-10-08

- Reflashed `four_motor_test` (15,406 B, verified), `READY,FOUR_MOTOR_PCA_ARM`,
  motors `STOPPED`. Servo poses released (arm must stay manually secured).
- Note: firmware `F` = physical reverse on M1–M3, forward on M4 (bench history).
  Use single-motor pulses to verify direction before chassis moves.

## Pick rest moved — 2026-10-08

- CH11 pick rest now 120° (1667 us), holding. (Was 60.) Servo fw 14,262 B back on Mega.

## Pick-and-drop sequence (agreed) — 2026-10-08

1. `C` pick pose: CH0 220 (2220) + CH1 60 (1335) + CH11 pick-ready 0 (1000).
2. `O` suction on (CH6 1500 + CH7 2000). Verified gripping.
3. `R` lift to 210 (2165), suction held, plate carried.
4. Base/drive adjust (needs four_motor reflash; pending).
5. Drop: CH1 staged to 220 (2220), then back to 60, CH0 to 220.
6. `L` release (CH6 2000 + CH7 1500), pose held.
7. `F` rest: 210 + 60 + 60.
- Resets clear everything: one-shot poses (C/F/Q/R/P) restore in one command.
- Flat-press optimization still open (past-220 or dwell).

## Pick-place plan saved — 2026-10-08

- Full plan in `PICK_PLACE_PLAN.md` (deferred until mecanum floor tests pass).

## Stale serial holder incident — 2026-10-08

- Aborting the opencode tool call does NOT kill the remote Pi process: the 67-step
  CH11 script kept running holding /dev/ttyACM0 (fuser showed PID 14182).
  Lesson: `pkill -f` the remote script (or `fuser -k`) before reusing the port.
- Killed PIDs, port freed, sent F + L: rest triple held, suction released.
- Pump ran at 2100 for several minutes during the incident — check pump heat.

## Bot 2 Mega base — 2026-10-08 (event-ready state)

- Bot 2 Pi (`hackathon-bot2`, Pi 4B Rev 1.5, Ubuntu 24.04, `192.168.1.10` DHCP)
  SSH key login verified; stale `known_hosts` entry for `.10` replaced.
  HAT PCA9685 at `0x5f` read-only probe: MODE1 `0x11`, MODE2 `0x04`, PRE `0x1E`.
- HAT servo tests: CH0 neutral hold, 0→180° stepped sweep, 3-cycle sine radar
  on CH0 / CH0+1 / CH0+1+2, then CH0–7 set to 0° hold. All exit 0, motors zeroed,
  prescale restored to `0x1E` after each test.
- HAT M4 port dead: CH8/9 writes verify on readback but no motion on FWD/REV/50%/
  13 s pulses. Only M1–M3 usable on HAT. No spare DC terminals on V3.3 (4x DC +
  16x servo-signal only).
- Base moved to Mega 2560 (CH340 clone, `/dev/ttyUSB0`) + 2x external L298N
  drivers using the Bot1 pin map (`arduino/four_motor_test`, 15,406 B flashed
  and verified via `avrdude -c wiring -b 115200` after `sudo apt install avrdude`
  on Bot2; hex at Pi `~/bot2/four_motor_bot2.hex`).
- Pin map: M1 D24/D25/PWM D11 Enc D19/D18; M2 D22/D23/D10 Enc D2/D35;
  M3 D26/D27/D9 Enc D3/D37; M4 D28/D29/D8 Enc D17(polled)/D39.
  D20/D21 I2C, D7 PCA OE, common GND. Bot2 Mega reports PCA `NOT_FOUND` (expected).
- All-four `F` pulses repeatedly ~M1 1990 / M2 2060 / M3 1880 / M4 1965 edges,
  B balanced. Intermittent faults seen and fixed by reseat: M2 B→D35, M3 B→D37
  (was sharing D35), M4 B→D39, driver power/GND. Occasional all-zero runs =
  motor battery sag/off. No firmware pin change (M2 A stays D2).
- Bot2 USB lesson: CH340 spewed garbage + re-enumerated (Device 011→016) on one
  Pi USB port; Mega proven healthy on PC (`READY` clean). USB2 port + reseat
  fixed it. If serial returns nulls, suspect cable/port/power before the board.
- Safe HAT script: `bot2/motor_sweep.py` (stdlib-only, one-at-a-time ~30 % 1 s
  pulses, auto-stop, signal-safe). Mega tests use `M1F`..`M4F`/`F`/`STOP` at
  115200, PWM 200, 1 s auto-stop, wheels raised.
- Event access: Bot1 `sanjeev@192.168.1.8`, Bot2 `sanjeev@192.168.1.10` (DHCP,
  may change; try `hackathon-bot2.local`). Same Wi-Fi as SPC event network.
  New PC must `ssh-copy-id` its key to both Pis; do NOT copy private keys.

## Bot 2 HAT servo session — 2026-10-08/09

- CH0 neutral hold, stepped 0→180°, 3-cycle sine radar on CH0 / CH0+1 / CH0+1+2,
  then CH0–7 to 0° hold at 50 Hz. Single-position moves on CH1/CH2 after:
  0/90/180 verified, CH2 220° and 270° (~2500 us) held per user confirm
  (270°-class servo). 300° (~2667 us) and 360° (~3000 us) REFUSED: past the
  confirmed 270° max, end-stop stall/strip risk. HAT left at 50 Hz holding
  CH0–7 at 0° (CH2 last at 0°); motors zero. Restore PRE `0x1E` before HAT
  motor runs. All moves eased with auto-hold; exits 0 throughout.

## Bot 1 live checks — 2026-10-09

- Pi 5 SSH at `192.168.1.8`; Mega on `/dev/ttyACM0` reported
  `READY,BOT1_ARM_BASE2_V14` and PCA9685 at `0x41`. No firmware upload.
- Individual two-second PWM 100 tests stopped automatically and received an
  explicit `BASESTOP`: M1 1,966 A edges, M2 2,228, M3 2,012, M4 2,099.
  Only the commanded motor's A count advanced in each test. M2 B stayed HIGH
  at all 2,228 A edges, so its signed direction count was unusable.
- A subsequent all-four `DRIVE2` completed its two-second timeout and explicit
  `BASESTOP`: M1 2,005, M2 2,153, M3 2,047, M4 2,093 A edges. M2 B again
  stayed HIGH at every A edge; the other three B signals were balanced.
  User had said the first all-four pulse showed no visible motor motion;
  physical observation of the later individual/all-four pulses is pending.
- Arm: CH1 was ramped from saved 60° to 210° (2165 us), then tested
  210°→220°→210° (2165→2220→2165 us). CH11 was tested
  60°→120°→0°→120° (1335→1667→1000→1667 us). CH6/CH7 received a one-second
  suction command and a one-second release command, followed by `GRIPOFF`.
  Mega acknowledged commands; physical movement/suction feedback is pending.
  Final `STATUS`: CH0 2165 us, CH1 2165 us, CH11 1667 us holding; base stopped.
- Logitech C270 captured 640×480 frames. Startup frame was dark; after warmup,
  a clear frame had grayscale mean 126.0 and standard deviation 35.8, saved on
  Pi as `/tmp/bot1-camera-check-warm.jpg` and PC as the same path.
- Later, user confirmed visible CH11 motion during a repeated gradual
  120°→0°→120° test. Final hold was CH11 1667 us; CH0/CH1 held 2165 us,
  gripper off, base stopped.
- User requested a straight base run. CH1 and CH11 were gradually moved to the
  firmware-required rest 1335/1335 us, `DRIVE2` ran for 2000 ms, then explicit
  `BASESTOP` was acknowledged. A edges: M1 658, M2 943, M3 1356, M4 767.
  Final measured rates were 750/770/1020/740 edges/s, all at PWM 140 limit;
  this did not meet the 1100 edges/s target or prove straight travel. M2 and M3
  B samples were uneven. CH1/CH11 were returned gradually to 2165/1667 us;
  final `STATUS` confirmed CH0/CH1/CH11 holds 2165/2165/1667, gripper off,
  base stopped. Physical travel observation is pending.

## Bot 2 Mega motor check — 2026-10-09

- Bot 2 Pi SSH `192.168.1.10`; CH340 Mega reappeared at `/dev/ttyUSB0`
  (stable `/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0`). It reported
  `READY,FOUR_MOTOR_PCA_ARM`, `STOPPED`, and PCA `NOT_FOUND` as expected.
- User confirmed wheels clear and power stop accessible. One-second PWM 200
  individual F pulses all reached automatic `STOPPED,TIMEOUT` and received
  explicit `STOP`: M1 1,693 A edges, M2 1,703, M3 1,610, M4 1,703.
  B high/low samples were balanced on each commanded motor.
- During the M2-only pulse, M1 also registered 302 A edges, with balanced B
  samples. Other off-target motors stayed at zero. A separate five-second idle
  check after encoder reset showed zero edges on all motors, and M3-only and
  M4-only pulses had zero off-target counts. The cause of M1 activity during
  M2 operation is unresolved; visual confirmation of which wheels moved is
  pending. No all-four pulse was run after this finding.
- User-requested M1/M2 rerun, same one-second PWM 200 pulses: M1 1,692 A
  edges with all other motors zero; M2 1,704 A edges with M1/M3/M4 all zero.
  Both B high/low counts were exactly balanced, both pulses auto-stopped, and
  explicit STOP was acknowledged. The M1 cross-count during the first M2 run
  did not recur in this retest; physical wheel observation is still pending.
- User-requested all-four `F` pulse at PWM 200 for one second auto-stopped and
  received explicit `STOP`. A edges M1 1,685, M2 1,701, M3 1,618, M4 1,702;
  B high/low at A edges were 842/843, 851/850, 809/809, 851/851. All four
  signals were balanced in this simultaneous run. Physical direction and
  straightness still require user observation.
- Bot 2 physical direction mapping started one motor at a time. User identified
  M1 as the front-right wheel and confirmed `M1F` spins it in the robot's
  forward-driving direction. Its repeat pulse produced 1,691 A edges, with
  all other wheels at zero. User corrected M2 identification to front left
  (an initial rear-right description was retracted) and confirmed `M2F` spins
  it backward. Multiple M2F repeats were isolated; latest 1,707 A edges,
  other wheels zero. User identified M3 as rear left and confirmed `M3F`
  spins it backward; its repeat produced 1,619 A edges, others zero. M4F
  direction-identification pulse produced 1,703 A edges, others zero; user
  identified it as rear right and confirmed `M4F` spins it forward.
- Confirmed Bot 2 physical mapping for the current Mega `F` polarity:
  M1 front right/forward, M2 front left/backward, M3 rear left/backward,
  M4 rear right/forward. At this stage, the old all-motor firmware `F` was not
  chassis straight forward. A logical straight-forward command had to reverse
  M2 and M3 relative to M1 and M4; the firmware update below implements this.

## Bot 2 logical-forward firmware — 2026-10-09

- Created Bot 2-specific source `bot2/arduino/bot2_four_motor/bot2_four_motor.ino`
  from the verified four-motor test firmware. `F` now commands mixed electrical
  polarities M1 F, M2 R, M3 R, M4 F; `R` inverts them. Individual `M1F`–`M4R`
  retain raw electrical polarity. The 1-second PWM 200 timeout, STOP, encoder
  reporting, and startup motor-off behavior are unchanged.
- Compiled for Mega 2560 (15,424 flash bytes). Backed up prior Mega flash on
  Bot 2 Pi to `~/bot2/pre_logical_forward_20261009.hex`, then uploaded
  `~/bot2/bot2_logical_v1.hex`; AVRDUDE verified all 15,424 bytes. Banner is
  `READY,BOT2_LOGICAL_BASE_V1`. Post-flash `STATUS` was `STOPPED`.
- One `F` raised-wheel pulse completed the automatic one-second timeout and
  explicit STOP. Encoder ticks/A edges were M1 +1690/1690, M2 -1713/1713,
  M3 -1624/1624, M4 +1700/1700; all B high/low samples were balanced. User
  confirmed all four wheels physically rotated forward. A subsequent `R`
  pulse also auto-stopped and received explicit STOP, with opposite signed
  ticks M1 -1708, M2 +1705, M3 +1616, M4 -1711 and balanced B samples.
  Physical reverse observation is pending. Floor straightness remains untested.

## Bot 2 first two-second ground pulse — 2026-10-09

- Added a separate `F2` logical-forward command to the Bot 2 sketch: PWM 120,
  2000 ms automatic timeout. The original `F`/`R` and individual commands
  remain one-second PWM 200 bench pulses. Compiled V2 (15,600 flash bytes),
  uploaded as Pi `~/bot2/bot2_logical_v2_ground.hex`, and AVRDUDE verified all
  bytes. V1 remains on Pi as `~/bot2/bot2_logical_v1.hex`. Banner is
  `READY,BOT2_LOGICAL_BASE_V2`; pre-run STATUS was STOPPED.
- With Bot 2 on the ground, ran one `F2` pulse. Firmware reported
  `START,GROUND,F,PWM120,2000MS`, automatic `STOPPED,TIMEOUT`, and explicit
  STOP. Encoder ticks/A edges: M1 +1517/1517, M2 -2087/2087,
  M3 -1878/1878, M4 +1733/1733; B high/low samples were balanced on all four.
  Count spread was substantial under load; visual straightness and travel
  distance are pending the user's observation.
- At user's request, reduced only `F2` from PWM 120 to PWM 110 (V3); bench
  commands stay PWM 200/1 s. Compiled 15,600 bytes, flashed Pi
  `~/bot2/bot2_logical_v3_ground.hex`, AVRDUDE verified, and banner confirmed
  `READY,BOT2_LOGICAL_BASE_V3`. The two-second PWM 110 ground rerun auto-stopped
  and received explicit STOP. A edges/ticks were M1 +1834, M2 -1972,
  M3 -1992, M4 +1880; all B high/low samples were exactly balanced. The count
  spread was smaller than in the PWM 120 run. The user confirmed the forward
  ground run worked well.
- Added matching low-speed `R2` for two-second PWM 110 logical reverse in V4;
  compiled 15,632 bytes, flashed Pi `~/bot2/bot2_logical_v4_ground.hex`, and
  AVRDUDE verified all bytes. A ground `R2` pulse reported
  `START,GROUND,R,PWM110,2000MS` and `STOPPED,TIMEOUT`; an explicit `STOP` and
  `STATUS` both reported `STOPPED`. The test script did not retain the encoder
  lines from this run. Physical reverse travel awaits the user's observation.
- Added `TR1` to V5 for a right turn in place: left M2/M3 logical forward,
  right M1/M4 logical reverse, PWM 110 for 1000 ms. Compiled 15,942 bytes,
  flashed Pi `~/bot2/bot2_logical_v5_turn.hex`, and AVRDUDE verified all bytes.
  The ground pulse reported `START,TURN,RIGHT,PWM110,1000MS`, automatic
  `STOPPED,TIMEOUT`, and explicit `STOPPED` twice. Encoder ticks/A edges were
  M1 -759/759, M2 -663/663, M3 -909/909, M4 -945/945; all B samples were
  balanced. Physical right-turn observation awaits the user's confirmation.
- Added matching `TL1` to V6 for a left turn in place: right M1/M4 logical
  forward, left M2/M3 logical reverse, PWM 110 for 1000 ms. Compiled 16,032
  bytes, flashed Pi `~/bot2/bot2_logical_v6_turn.hex`, and AVRDUDE verified.
  The ground pulse reported `START,TURN,LEFT,PWM110,1000MS`, automatic
  `STOPPED,TIMEOUT`, and explicit `STOPPED` twice. Encoder ticks/A edges were
  M1 +703/703, M2 +961/961, M3 +908/908, M4 +776/776; B samples were
  balanced. The user confirmed the left turn worked.
- Added V7 lateral `SR1`/`SL1` commands using the conventional X-roller
  mecanum pattern, PWM 140 for 1000 ms. Compiled 16,380 bytes, flashed Pi
  `~/bot2/bot2_logical_v7_lateral.hex`, and AVRDUDE verified. The first
  ground `SR1` pulse reported `START,LATERAL,RIGHT,PWM140,1000MS`, automatic
  `STOPPED,TIMEOUT`, and explicit `STOPPED` twice. Encoder ticks/A edges were
  M1 -177/177, M2 -477/477, M3 +482/482, M4 +529/529; B samples were
  balanced. M1 travelled substantially less than the other wheels. Actual
  rightward motion awaits the user's observation; `SL1` has not been run.
- At the user's request, V8 added two-second `SR2`/`SL2` lateral commands,
  keeping PWM 140 and the one-second commands. Compiled 16,488 bytes, flashed
  Pi `~/bot2/bot2_logical_v8_lateral.hex`, and AVRDUDE verified. The `SR2`
  ground retry reported `START,LATERAL,RIGHT,PWM140,2000MS`, automatic
  `STOPPED,TIMEOUT`, and explicit `STOPPED` twice. Encoder ticks/A edges were
  M1 -1284/1284, M2 -1566/1566, M3 +1421/1421, M4 +1450/1450; all B samples
  were balanced. M1 was closer to the other wheels than in the one-second
  test. Physical sideways motion awaits the user's observation.
- Ran the matching two-second PWM 140 `SL2` ground pulse without another
  firmware change. It reported `START,LATERAL,LEFT,PWM140,2000MS`, automatic
  `STOPPED,TIMEOUT`, and explicit `STOPPED` twice. Encoder ticks/A edges were
  M1 +1609/1609, M2 +2068/2068, M3 -1872/1872, M4 -1994/1994; B samples
  were balanced. Physical leftward motion awaits the user's observation.
- At the user's request, repeated the two-second PWM 140 `SR2` pulse. It
  auto-stopped and received explicit STOP/STATUS confirmations. Encoder
  ticks/A edges were M1 -1696/1696, M2 -2002/2002, M3 +1620/1620,
  M4 +2081/2081, with balanced B samples. The user confirmed physical
  rightward motion.
- After clarifying the requested direction, repeated two-second PWM 140
  `SL2`. It reported `START,LATERAL,LEFT,PWM140,2000MS`, automatic
  `STOPPED,TIMEOUT`, and explicit STOP/STATUS confirmations. Encoder ticks/A
  edges were M1 +1628/1628, M2 +1949/1949, M3 -1894/1894, M4 -1969/1969;
  B samples were balanced. Physical leftward motion awaits the user's report.

## Bot 2 session checkpoint and HAT servo CH0 — 2026-10-09

- Bot 2 Mega remains on verified V8 firmware, with base commands `F2/R2`
  (2 s PWM 110), `TR1/TL1` (1 s PWM 110), and `SR1/SL1/SR2/SL2`
  (1/2 s PWM 140). All logged runs auto-stopped, and the latest lateral-left
  run also received explicit STOP/STATUS confirmations. User confirmed forward,
  left turn, and lateral-right travel; physical reverse, right-turn, and
  lateral-left observations were not explicitly confirmed in this record.
- User moved on to Bot 2's Adeept HAT servo tests, one channel at a time.
  Read-only I2C probe found only PCA9685 `0x5f` among `0x40/0x41/0x5f`.
  Before actuation, HAT MODE1 was `0x11`, MODE2 `0x04`, prescale `0x1e`, and
  all 16 outputs were full-off. `i2cdetect` and Python `smbus` packages were
  absent, so stdlib `fcntl` I2C access was used.
- At user request, set HAT CH0 to Adeept vendor nominal 0°: 500 us at 50 Hz,
  PCA9685 off count 102. Script is `bot2/hat_servo_ch0_zero.py`, copied to Pi
  `~/bot2/hat_servo_ch0_zero.py`. Post-command readback was MODE1 `0x21`,
  MODE2 `0x04`, prescale `0x79`, CH0 `[0,0,102,0]`; CH1–CH15 remained
  full-off. Physical CH0 position/motion awaits user observation.
- At user request, gradually moved HAT CH0 from nominal 0° to nominal 180°
  using the Adeept 500–2400 us mapping at 50 Hz, one degree every 20 ms.
  Script is `bot2/hat_servo_ch0_to_180.py`, copied to Pi. It verified CH0's
  0° starting readback and that CH1–CH15 were off before moving. It reported
  `HOLDING,CH0,DEG180,PULSE2400US,COUNT492`; readback was MODE1 `0x21`,
  prescale `0x79`, CH0 `[0,0,236,1]`. CH0 PWM is now holding nominal 180°;
  base motors remain stopped. Physical CH0 position awaits user confirmation.
  Do not change HAT frequency or drop CH0 hold without checking support.
- At user request, gradually returned HAT CH0 from nominal 180° to 0° using
  one-degree steps every 20 ms. Script `bot2/hat_servo_ch0_to_0.py` verified
  the 180° starting readback and all other outputs off before moving. It
  reported `HOLDING,CH0,DEG0,PULSE500US,COUNT102`; final readback was MODE1
  `0x21`, prescale `0x79`, CH0 `[0,0,102,0]`. CH0 now holds nominal 0°.
- At user request, set HAT CH1 to vendor nominal 0° with isolated script
  `bot2/hat_servo_ch1_zero.py`. It verified CH0 still held 0° and CH2–CH15
  were off before enabling CH1. Readback confirmed CH0 and CH1 both
  `[0,0,102,0]` at prescale `0x79` (50 Hz). Both channels now hold nominal
  0°; physical CH1 position awaits user observation.
- Interpreted the user's request for a slanting position as CH1 nominal 45°.
  Script `bot2/hat_servo_ch1_to_45.py` verified CH0/CH1 at 0° and CH2–CH15
  off, then stepped CH1 by one nominal degree every 30 ms. It reported
  `HOLDING,CH1,DEG45,PULSE975US,COUNT200`; readback was CH0 `[0,0,102,0]`,
  CH1 `[0,0,200,0]`, prescale `0x79`. CH1 now holds nominal 45°; physical
  slant awaits user observation.
- User asked to keep CH1 at zero. Script `bot2/hat_servo_ch1_to_0.py` moved
  it gradually from nominal 45° back to 0° and read back CH0/CH1 both
  `[0,0,102,0]` at prescale `0x79`. Both CH0 and CH1 now hold nominal 0°.
- User then requested CH1 nominal 180°. Prepared
  `bot2/hat_servo_ch1_to_180.py` to check the recorded CH0/CH1 zero holds,
  keep CH2–CH15 off, and move CH1 from 0° to 180° in 20 ms steps. The script
  was copied to Pi `~/bot2/hat_servo_ch1_to_180.py`. Its remote execution was
  interrupted before output was returned, so completion and final CH1 pulse
  were not electronically confirmed. The user subsequently reported "all
  good" and asked to save the session. A read-only verification attempt then
  failed with `No route to host` for `192.168.1.10`. Last confirmed HAT
  readback remains CH0/CH1 both nominal 0°; actual current CH1 state needs
  a fresh read when the Pi is reachable. No further motion was commanded.
