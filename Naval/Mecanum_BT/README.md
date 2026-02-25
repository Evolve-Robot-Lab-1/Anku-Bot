# Mecanum Wheel Robot - Bluetooth Control

## Overview
4-wheel mecanum robot controlled via Bluetooth (HC-05) or USB Serial.
Runs on Arduino Mega. Open-loop control (no encoders).

## Hardware

### Motors & Drivers

| Motor | Position | Driver | Pins |
|-------|----------|--------|------|
| FL | Front Left | L293D | EN=8, IN1=40, IN2=41 |
| FR | Front Right | L293D | EN=2, IN1=22, IN2=23 |
| BL | Back Left | L293D | EN=4, IN1=7, IN2=9 |
| BR | Back Right | BTS7960 | RPWM=10, LPWM=11 |

### Bluetooth (HC-05)

| HC-05 Pin | Arduino Mega Pin |
|-----------|-----------------|
| TXD | Pin 17 (RX2) |
| RXD | Pin 16 (TX2) |

### Baud Rates
- USB Serial: 115200
- Bluetooth (Serial2): 9600

## Wheel Layout

```
Top view:
  FL \  / FR
  BL /  \ BR
```

Standard mecanum X-pattern (rollers form an X when viewed from top).

## Commands

| Key | Action |
|-----|--------|
| F | Forward |
| B | Backward |
| L | Strafe Left |
| R | Strafe Right |
| W | Rotate Left (CCW) |
| U | Rotate Right (CW) |
| S | Stop |
| 1 | Speed 120 (slow) |
| 2 | Speed 180 (default) |
| 3 | Speed 230 (fast) |

Commands are case-insensitive (F and f both work).

## Motor Direction Map

| Command | FL | FR | BL | BR |
|---------|----|----|----|----|
| Forward | +1 | +1 | +1 | +1 |
| Backward | -1 | -1 | -1 | -1 |
| Strafe Left | -1 | +1 | +1 | -1 |
| Strafe Right | +1 | -1 | -1 | +1 |
| Rotate Left | -1 | +1 | -1 | +1 |
| Rotate Right | +1 | -1 | +1 | -1 |

+1 = Forward, -1 = Backward

## Notes
- Open-loop control: No encoders, no PID. Manual control only.
- BR uses BTS7960 (different driver than the other 3 L293D motors). PWM response may differ slightly.
- If robot drifts during straight movement, adjust individual motor speeds to compensate.
- Default speed is 180 on startup.

## Last Updated
Feb 10, 2026
