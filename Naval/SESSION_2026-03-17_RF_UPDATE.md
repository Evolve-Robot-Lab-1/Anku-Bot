# Session Update - 2026-03-17

## Scope

Added FlySky RF receiver support to the Naval robot project as a non-breaking extension to the existing Ethernet + RPi + ROS + web + joystick workflow.

## What Was Completed

- Saved RF integration plan in:
  - [RF_INTEGRATION_PLAN.md](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/RF_INTEGRATION_PLAN.md)
- Saved repo-local RF wiring/setup note in:
  - [FLYSKY_CT6B_FS_R6B_SETUP.md](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/FLYSKY_CT6B_FS_R6B_SETUP.md)
- Patched active Arduino firmware:
  - [Mecanum_Encoder.ino](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/Mecanum_Encoder/Mecanum_Encoder.ino)
- Added RF control source arbitration:
  - RF is additive, does not replace ROS serial
  - existing serial protocol preserved
  - CH5-based arming/disarming logic added
- Added stricter RF safety gate:
  - RF must see disarm first
  - sticks must be centered
  - arm request must be held stable before RF control is accepted

## Firmware / Deployment Status

- Local compile on development machine: passed
- Copied updated sketch to robot Pi
- Installed `arduino-cli` on robot Pi in:
  - `/home/pi/bin/arduino-cli`
- Installed Arduino AVR core on robot Pi
- Uploaded updated firmware from robot Pi to Mega on:
  - `/dev/ttyACM0`
- Restarted robot service successfully

## Robot Pi Status

- Robot Pi reachable over Ethernet at:
  - `10.0.0.2`
- `naval.service` restarted successfully after reflashing
- Web health endpoint working:
  - `http://10.0.0.2:5000/health`

## Existing Workflow Verification

- Existing joystick control still works after firmware update
- That confirms:
  - Pi-to-Mega serial path still works
  - ROS stack still works
  - web/joystick workflow survived the RF firmware changes

## RF Wiring Used In Firmware

- Receiver `VCC` -> Mega `5V`
- Receiver `GND` -> Mega `GND`
- `CH1` -> `D12`  (PB6, PCINT6)
- `CH2` -> `D11`  (PB5, PCINT5) — moved off D13 (LED)
- `CH3` -> `A8`   (PK0, PCINT16) — moved off D46 (no PCINT)
- `CH4` -> `A9`   (PK1, PCINT17) — moved off D47 (no PCINT)
- `CH5` -> `A10`  (PK2, PCINT18) — moved off D48 (no PCINT)

**IMPORTANT**: These pins changed from the March 17 session. Receiver must be rewired.

## RF Debug Findings

Earlier logs showed the receiver was producing values such as:

- `CH1 ~ 1500`
- `CH5` sometimes high enough to arm (`1716`, `1752`, `1792`)
- `CH5` often mid/undefined (`1452` to `1600`)
- `CH4` strongly off-center (`924` to `1060`) instead of near `1500`

Interpretation:

- The Pi and Arduino were reading receiver pulses at least at one stage
- `CH5` did not behave like a clean two-position disarm/arm channel
- `CH4` looked non-neutral and was a likely cause of unintended motion

## Isolation Work Done

- User later isolated channels for testing
- With only `CH5` connected, no fresh useful RF events were captured
- With only `CH1` connected and moved, no fresh useful RF events were captured
- Receiver LED was reported as solid

## Current Most Likely Open Problem

The remaining issue is likely on the receiver/transmitter wiring or signal orientation side, not on the Pi launch workflow.

Most likely candidates:

- signal wire orientation on receiver channel pins
- wrong row used on receiver (`S` vs `+` vs `-`)
- actual channel output assignment on transmitter differs from assumption
- channel 5 switch mapping on transmitter is not configured as a clean low/high output

## Safe Resume Point

When resuming later:

1. Keep robot wheels off ground
2. Re-verify receiver signal pin orientation
3. Re-test one receiver channel at a time
4. Start with isolated known channel verification before reconnecting all channels
5. Do not reconnect CH4 and CH5 together until isolated testing is clean

## Recommended Next Debug Order

1. Verify receiver pin orientation physically (`S`, `+`, `-`)
2. Test isolated channel outputs one by one
3. Confirm which transmitter control actually drives channel 5
4. Confirm CH4 neutral value near `1500`
5. Only then reconnect full RF wiring

