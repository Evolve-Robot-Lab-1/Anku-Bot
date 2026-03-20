# FlySky CT6B + FS-R6B Integration for Naval Robot

This version is aligned to the current repo runtime, not the generic Mega wiring note.

## Current Runtime Path

The active launch flow in this repo is:

- PC launcher: [launch_naval.sh](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/launch_naval.sh)
- Pi launcher: [start_naval.sh](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/naval_web_control/start_naval.sh)
- ROS serial bridge: [naval_serial_bridge.py](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/naval_web_control/naval_web_control/naval_serial_bridge.py)

Important consequence:

- Raspberry Pi talks to Arduino over USB serial `/dev/ttyACM*`
- Baud rate is `115200`
- The current runtime does not use Mega `Serial1` for Pi communication

## Arduino Firmware Expected by ROS Stack

The ROS serial bridge sends these commands:

- `F/B/L/R/W/U/S`
- `1/2/3`
- `T<angle>`
- `G<angle>`

The Arduino sketch in this repo that matches that command set is:

- [Mecanum_Encoder.ino](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/Mecanum_Encoder/Mecanum_Encoder.ino)

That makes it the best candidate for the current deployed firmware interface.

## Pin Usage From Current Arduino Sketch

From `Mecanum_Encoder.ino`:

### Motors

| Function | Mega Pin |
|---|---|
| M1 EN | D2 |
| M1 IN1 | D22 |
| M1 IN2 | D23 |
| M2 EN | D3 |
| M2 IN1 | D24 |
| M2 IN2 | D25 |
| M3 EN | D5 |
| M3 IN1 | D28 |
| M3 IN2 | D29 |
| M4 EN | D4 |
| M4 IN1 | D26 |
| M4 IN2 | D27 |

### Encoders

| Function | Mega Pin |
|---|---|
| M1 CHA | D18 |
| M1 CHB | D30 |
| M2 CHA | D19 |
| M2 CHB | D31 |
| M3 CHA | D21 |
| M3 CHB | D33 |
| M4 CHA | D20 |
| M4 CHB | D32 |

### Camera Servos

| Function | Mega Pin |
|---|---|
| Front tilt servo | D9 |
| Rear tilt servo | D10 |

### USB Serial

| Function | Mega Pin |
|---|---|
| USB debug and ROS serial bridge | `Serial` over USB |

## Receiver Wiring Recommendation

Do not use the generic note's receiver mapping on this project:

- `D2`, `D3`, `D4`, `D5` are already used
- `D18`, `D19`, `D20`, `D21` are already used by encoders
- `D9`, `D10` are used by camera servos

Recommended FS-R6B signal pins for this project (all PCINT-capable):

| FS-R6B | Mega Pin | Port/PCINT | Function |
|---|---|---|---|
| CH1 | D12 | PB6 / PCINT6 | Rotate |
| CH2 | D11 | PB5 / PCINT5 | Forward / backward |
| CH3 | A8 | PK0 / PCINT16 | Speed scale |
| CH4 | A9 | PK1 / PCINT17 | Lateral strafe |
| CH5 | A10 | PK2 / PCINT18 | Arm / disarm |

NOTE: CH2 was moved off D13 (built-in LED attenuates signal).
CH3-CH5 were moved off D46-D48 (no PCINT support on Port L).

Power:

- Receiver `VCC` -> Mega `5V`
- Receiver `GND` -> Mega `GND`

## Why These Pins

These pins avoid the current motor, encoder, servo, and USB serial wiring used by the repo.

All five pins support Pin Change Interrupts (PCINT) on the ATmega2560:
- D11, D12 are on Port B (PCINT0 vector)
- A8, A9, A10 are on Port K (PCINT2 vector)

The firmware uses raw PCINT ISRs (no external library needed) for reliable
pulse capture that won't miss edges during Serial prints or PID updates.

## Channel Mapping

| Stick | Channel | Function |
|---|---|---|
| Left stick Up/Down | CH2 | Forward / Backward |
| Left stick Left/Right | CH4 | Lateral strafe |
| Right stick Left/Right | CH1 | Rotate |
| Right stick Up/Down | CH3 | Speed scale |
| Switch | CH5 | Arm / stop |

## Motion Mix

```text
FL = throttle + lateral + rotate
FR = throttle - lateral - rotate
RL = throttle - lateral + rotate
RR = throttle + lateral - rotate
```

## Arm / Stop Logic

Recommended:

- `CH5 > 1700 us` -> armed
- `CH5 < 1300 us` -> disarmed and force stop

Disarmed state should always override:

- FlySky stick input
- web UI commands
- ROS joystick commands

## Recommended Control Priority

For this project, the safest control order is:

1. `CH5 disarmed` -> hard stop
2. active RF input -> local manual control on Mega
3. otherwise accept ROS serial commands from Pi

This lets the transmitter act as an immediate local override without breaking the existing web and joystick control pipeline.

## Important Mismatch in Repo

There is an older working pin map in [PIN_MAP.md](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/PIN_MAP.md) that differs from `Mecanum_Encoder.ino`.

That means the repo currently contains at least two hardware generations:

- one reflected in `PIN_MAP.md`
- another reflected in `Mecanum_Encoder.ino`

The launch scripts do not name a specific `.ino` file. They only assume an Arduino is present on USB and understands the serial protocol above.

Because the ROS bridge sends `T...` and `G...` tilt commands, `Mecanum_Encoder.ino` is the closest firmware match to the active software stack.

## Binding Reminder

1. Power off receiver and transmitter
2. Hold receiver bind button while powering receiver
3. Power on CT6B transmitter
4. Wait for receiver LED to go solid
5. Power cycle and test channels

## Grounding Rule

All of these must share ground:

- Arduino Mega
- FS-R6B receiver
- motor drivers
- battery supply
- Raspberry Pi if USB-grounded path is not already common
