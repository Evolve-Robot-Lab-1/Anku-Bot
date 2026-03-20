# RF Integration Plan

Goal: add FlySky CT6B + FS-R6B transmitter support to the existing Naval robot project without breaking the current Ethernet + Raspberry Pi + ROS + web/joystick workflow.

## Constraints

- Keep the existing launch flow unchanged
- Keep the current Arduino serial protocol unchanged
- Do not break:
  - ROS serial bridge
  - web UI
  - joystick bridge
  - HDMI kiosk
  - camera tilt commands

## Active Baseline

The current project runtime expects the Arduino to speak this serial protocol over USB:

- `F/B/L/R/W/U/S`
- `1/2/3`
- `T<angle>`
- `G<angle>`

The best firmware target in this repo is:

- [Mecanum_Encoder.ino](/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/Naval/Mecanum_Encoder/Mecanum_Encoder.ino)

## Integration Strategy

1. Preserve the existing ROS serial path exactly.
2. Add FlySky receiver input as a second control source inside Arduino.
3. Use source arbitration in firmware, not in ROS.
4. Keep Pi-side launch scripts and ROS nodes unchanged.

## Control Priority

1. CH5 disarmed -> hard stop
2. RF signal valid and armed -> RF controls drive
3. Otherwise -> keep using ROS serial commands

## Safety Rules

- RF timeout must release control cleanly
- CH5 disarm must stop motors immediately
- Invalid PWM pulses must be ignored
- Serial commands must continue to work when RF is absent

## Firmware Work

- Add receiver pin definitions
- Add PWM pulse measurement for CH1-CH5
- Map channels to:
  - CH1 rotate
  - CH2 throttle
  - CH3 speed scale
  - CH4 strafe
  - CH5 arm/disarm
- Add source switching diagnostics
- Refactor movement handling so repeated commands do not restart calibration unnecessarily

## Test Sequence

1. Compile firmware
2. Boot with receiver disconnected and verify current ROS/web behavior is unchanged
3. Connect receiver and verify CH5 disarm always stops motors
4. Verify RF driving while armed
5. Verify releasing RF or losing signal returns control to ROS
6. Verify tilt commands still work from Pi/web
7. Verify reboot and normal launch scripts still work
