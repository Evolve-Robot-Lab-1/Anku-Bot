# Bot 2 — Raspberry Pi 4B and Adeept Robot HAT V3.3

Status (2026-10-08): Pi SSH + HAT PCA9685 verified; HAT M4 port dead;
base now runs on a Mega 2560 + 2x external drivers (Bot1 pin map).
See SESSION_NOTES.md "Bot 2 Mega base" for the verified state.

## Confirmed hardware

- Raspberry Pi 4B (user confirmed).
- Adeept Robot HAT V3.3 (user confirmed). Four DC motor ports M1–M4 are available on the HAT.
- Other Bot 2 devices, motor voltage/current, wheel mapping, encoders, OS, and access details are still unknown.

## Control architecture

The Pi's 40-pin header connects to the HAT. The motors are controlled through the HAT's PCA9685 over the Pi's I²C bus, **not by driving motor power from bare Pi GPIO**. Adeept's supplied `05_Motor.py` uses I²C address `0x5f` and these PCA9685 channel pairs:

| HAT motor port | PCA9685 IN1 | PCA9685 IN2 | Pi connection |
| --- | ---: | ---: | --- |
| M1 | 15 | 14 | I²C bus |
| M2 | 12 | 13 | I²C bus |
| M3 | 11 | 10 | I²C bus |
| M4 | 8 | 9 | I²C bus |

Pi I²C uses BCM GPIO2/SDA (physical pin 3) and GPIO3/SCL (physical pin 5). Verify the actual HAT responds at `0x5f` before sending PWM. Keep PCA9685 channels 8–15 for motors. Adeept's example assigns servo channels 0–7 when motors are also used; reserve them until Bot 2's other actuators are identified.

The HAT also exposes direct Pi GPIO for sensors. Adeept's tutorial identifies GPIO23/24 for ultrasonic trigger/echo, GPIO17/27/22 for line tracking, and GPIO10 for WS2812. These are **existing HAT allocations**, not a proposed wiring list. Before assigning a new device to a Pi GPIO, inventory every attached HAT connector and check for overlaps. Pi GPIO logic is 3.3 V; use level shifting for any 5 V signal into the Pi.

## Bring-up plan

1. Record a photo of both sides of the V3.3 HAT, motor and battery labels, and every attached cable. Identify M1–M4 by physical wheel position. Record the Pi OS version and how to reach it by SSH.
2. With motor power off, inspect the HAT seating, motor terminal polarity, battery/Vin wiring, grounds, and accessible power stop. Use a supply rated for the motors. Follow Adeept's power guidance; do not power the HAT through its USB-C and Vin inputs simultaneously.
3. Boot the Pi without commanding motion. Check `/dev/i2c-1`, enable I²C if needed, and scan for `0x5f`. Record all I²C addresses and look for conflicts. Install only the Python packages needed for the confirmed OS in an isolated environment; avoid blindly running the vendor `setup.py` with `sudo`.
4. Make a Bot 2 motor test that initializes all four channels at zero, pulses **one** motor at low duty for a short fixed interval, and returns all channels to zero in `finally` and on `SIGINT`/`SIGTERM`. Test with wheels raised and a physical power stop available. Log port, commanded direction, observed wheel, and whether it stops. Repeat for M1–M4, then verify reverse direction one at a time.
5. Only after individual tests pass, run a short four-motor pulse. Define logical forward directions from actual wheel rotation; do not assume the vendor M1–M4 order matches the chassis.
6. Identify any wheel encoders and their output voltage. Allocate free BCM GPIO inputs only after the HAT pin inventory; level-shift outputs above 3.3 V. Measure ticks per wheel turn before odometry or autonomous driving.
7. Build bounded manual driving with a deadman timeout, speed limits, explicit stop, and startup/shutdown zero output. Navigation is a later phase after direction, encoder, and stop behavior are verified.

## Decisions pending

- Which extra device should use a Pi GPIO pin? Record its model, signal type, voltage, required inputs/outputs, and preferred connector before choosing a pin.
- Bot 2 motor voltage/current, battery type, wheel layout, and whether encoders are fitted.
- Pi access details and OS image. Do not reuse Bot 1's SSH address or Mega pin map.

## Sources

- [Adeept Robot HAT product page](https://www.adeept.com/adeept-robot-hat-v30-for-raspberry-pi_p0429.html) — V3.3 feature list. The page mixes V3.1 and V3.3 text; the user confirmed the physical board is V3.3.
- [Adeept Robot HAT V3 tutorial and code bundle](https://www.adeept.com/learn/detail-84.html) — `05_Motor.py`, PCA9685 address and motor channels; `Introduction of Robot HAT_V3.pdf`, power and connector guidance. Confirm on the actual V3.3 board because the bundle also contains a V3.1 schematic.
- [Raspberry Pi GPIO documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#gpio-and-the-40-pin-header) — 40-pin header, GPIO2/3 I²C, and 3.3 V GPIO levels.
