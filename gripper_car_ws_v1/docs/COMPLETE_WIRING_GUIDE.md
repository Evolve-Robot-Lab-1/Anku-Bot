# Complete Wiring Guide - Mobile Manipulator Robot

> **Document Created:** 2026-01-23
> **Project:** Gripper Car Workspace V1
> **Hardware:** Arduino Mega 2560 + PCA9685 + 2x L298N + YDLiDAR X2L

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Arduino Mega 2560 Pinout Summary](#2-arduino-mega-2560-pinout-summary)
3. [Motor Driver Wiring (2x L298N)](#3-motor-driver-wiring-2x-l298n)
4. [Encoder Wiring](#4-encoder-wiring)
5. [PCA9685 Servo Driver Wiring](#5-pca9685-servo-driver-wiring)
6. [Arm Servo Connections](#6-arm-servo-connections)
7. [Raspberry Pi USB Connections](#7-raspberry-pi-usb-connections)
8. [Power Distribution](#8-power-distribution)
9. [Complete Wiring Diagram](#9-complete-wiring-diagram)
10. [Quick Reference Tables](#10-quick-reference-tables)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. System Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           SYSTEM ARCHITECTURE                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────┐        USB         ┌─────────────────────────┐    │
│  │  Raspberry Pi 4 │◄──────────────────►│   Arduino Mega 2560     │    │
│  │  192.168.1.7    │    /dev/ttyACM0    │                         │    │
│  │                 │    115200 baud     │   ┌─────────────────┐   │    │
│  │  /dev/ttyUSB0   │                    │   │  I2C (20,21)    │   │    │
│  │       │         │                    │   │       │         │   │    │
│  └───────┼─────────┘                    │   │       ▼         │   │    │
│          │                              │   │   PCA9685       │   │    │
│          ▼                              │   │   (6 Servos)    │   │    │
│  ┌───────────────┐                      │   └─────────────────┘   │    │
│  │  YDLiDAR X2L  │                      │                         │    │
│  │  (LiDAR)      │                      │   ┌─────────────────┐   │    │
│  └───────────────┘                      │   │  GPIO Pins      │   │    │
│                                         │   │       │         │   │    │
│                                         │   │       ▼         │   │    │
│                                         │   │  2x L298N       │   │    │
│                                         │   │  (4 Motors)     │   │    │
│                                         │   └─────────────────┘   │    │
│                                         └─────────────────────────┘    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Arduino Mega 2560 Pinout Summary

### Used Pins Overview

| Pin Type | Pins Used | Purpose |
|----------|-----------|---------|
| **Digital (Motor IN)** | 22-29 | L298N direction control |
| **PWM (Motor EN)** | 9, 10, 11, 12 | L298N speed control |
| **Interrupt (Encoder A)** | 2, 3, 18, 19 | Encoder pulse counting |
| **Digital (Encoder B)** | 31, 33, 35, 37 | Encoder direction |
| **I2C** | 20 (SDA), 21 (SCL) | PCA9685 communication |
| **USB** | - | Serial to Raspberry Pi |

### Pin Map Diagram

```
Arduino Mega 2560 Pin Usage:

DIGITAL PINS:
┌────────────────────────────────────────────────────────────┐
│  Pin 2  ─► BL Encoder A (INT0)                            │
│  Pin 3  ─► BR Encoder A (INT1)                            │
│  Pin 9  ─► BR Motor Enable (PWM)                          │
│  Pin 10 ─► BL Motor Enable (PWM)                          │
│  Pin 11 ─► FR Motor Enable (PWM)                          │
│  Pin 12 ─► FL Motor Enable (PWM)                          │
│  Pin 18 ─► FL Encoder A (INT3)                            │
│  Pin 19 ─► FR Encoder A (INT4)                            │
│  Pin 20 ─► I2C SDA (PCA9685)                              │
│  Pin 21 ─► I2C SCL (PCA9685)                              │
│  Pin 22 ─► FL Motor IN1                                   │
│  Pin 23 ─► FL Motor IN2                                   │
│  Pin 24 ─► FR Motor IN1                                   │
│  Pin 25 ─► FR Motor IN2                                   │
│  Pin 26 ─► BL Motor IN1                                   │
│  Pin 27 ─► BL Motor IN2                                   │
│  Pin 28 ─► BR Motor IN1                                   │
│  Pin 29 ─► BR Motor IN2                                   │
│  Pin 31 ─► FL Encoder B                                   │
│  Pin 33 ─► FR Encoder B                                   │
│  Pin 35 ─► BL Encoder B                                   │
│  Pin 37 ─► BR Encoder B                                   │
└────────────────────────────────────────────────────────────┘
```

---

## 3. Motor Driver Wiring (2x L298N)

### Motor Pin Mapping

| Motor | Position | IN1 | IN2 | EN (PWM) | L298N Board |
|-------|----------|-----|-----|----------|-------------|
| **FL** | Front Left | Pin 22 | Pin 23 | Pin 12 | Board #1 |
| **FR** | Front Right | Pin 24 | Pin 25 | Pin 11 | Board #1 |
| **BL** | Back Left | Pin 26 | Pin 27 | Pin 10 | Board #2 |
| **BR** | Back Right | Pin 28 | Pin 29 | Pin 9 | Board #2 |

### L298N Board #1 (Front Motors)

```
                    L298N Board #1
┌─────────────────────────────────────────────────┐
│                                                 │
│   12V INPUT        GND          5V OUTPUT       │
│      │              │              │            │
│      ○              ○              ○            │
│      │              │              │            │
│   Battery(+)    Common GND     (Not Used)       │
│                                                 │
├─────────────────────────────────────────────────┤
│                                                 │
│   IN1   IN2   IN3   IN4   ENA   ENB            │
│    │     │     │     │     │     │             │
│    ○     ○     ○     ○     ○     ○             │
│    │     │     │     │     │     │             │
│   22    23    24    25    12    11             │
│    └──┬──┘     └──┬──┘     │     │             │
│    FL Dir     FR Dir    FL PWM FR PWM          │
│                                                 │
├─────────────────────────────────────────────────┤
│                                                 │
│   OUT1  OUT2       OUT3  OUT4                  │
│    │     │          │     │                    │
│    ○     ○          ○     ○                    │
│    │     │          │     │                    │
│    └──┬──┘          └──┬──┘                    │
│   FL Motor         FR Motor                    │
│   (+ and -)        (+ and -)                   │
│                                                 │
└─────────────────────────────────────────────────┘
```

### L298N Board #2 (Rear Motors)

```
                    L298N Board #2
┌─────────────────────────────────────────────────┐
│                                                 │
│   12V INPUT        GND          5V OUTPUT       │
│      │              │              │            │
│      ○              ○              ○            │
│      │              │              │            │
│   Battery(+)    Common GND     (Not Used)       │
│                                                 │
├─────────────────────────────────────────────────┤
│                                                 │
│   IN1   IN2   IN3   IN4   ENA   ENB            │
│    │     │     │     │     │     │             │
│    ○     ○     ○     ○     ○     ○             │
│    │     │     │     │     │     │             │
│   26    27    28    29    10     9             │
│    └──┬──┘     └──┬──┘     │     │             │
│    BL Dir     BR Dir    BL PWM BR PWM          │
│                                                 │
├─────────────────────────────────────────────────┤
│                                                 │
│   OUT1  OUT2       OUT3  OUT4                  │
│    │     │          │     │                    │
│    ○     ○          ○     ○                    │
│    │     │          │     │                    │
│    └──┬──┘          └──┬──┘                    │
│   BL Motor         BR Motor                    │
│   (+ and -)        (+ and -)                   │
│                                                 │
└─────────────────────────────────────────────────┘
```

### Motor Direction Logic

| IN1 | IN2 | Motor Action |
|-----|-----|--------------|
| HIGH | LOW | Forward |
| LOW | HIGH | Reverse |
| LOW | LOW | Stop (Coast) |
| HIGH | HIGH | Stop (Brake) |

---

## 4. Encoder Wiring

### Encoder Pin Mapping

| Motor | Encoder A (Interrupt) | Encoder B (Direction) |
|-------|----------------------|----------------------|
| **Front Left** | Pin 18 (INT3) | Pin 31 |
| **Front Right** | Pin 19 (INT4) | Pin 33 |
| **Back Left** | Pin 2 (INT0) | Pin 35 |
| **Back Right** | Pin 3 (INT1) | Pin 37 |

### Encoder Wiring Diagram

```
Each Motor Encoder (4 total):

┌─────────────────────┐
│   ENCODER MODULE    │
│                     │
│   VCC ──────────────┼──────► Arduino 5V
│                     │
│   GND ──────────────┼──────► Arduino GND
│                     │
│   CH_A (Output A) ──┼──────► Interrupt Pin (2/3/18/19)
│                     │
│   CH_B (Output B) ──┼──────► Digital Pin (31/33/35/37)
│                     │
└─────────────────────┘


All 4 Encoders Connected:

Arduino Mega                    Encoders
┌──────────────┐         ┌─────────────────┐
│              │         │   FL Encoder    │
│  5V ─────────┼────┬────┤ VCC             │
│              │    │    │                 │
│  Pin 18 ─────┼────│────┤ CH_A            │
│  Pin 31 ─────┼────│────┤ CH_B            │
│              │    │    └─────────────────┘
│              │    │    ┌─────────────────┐
│              │    │    │   FR Encoder    │
│              │    ├────┤ VCC             │
│  Pin 19 ─────┼────│────┤ CH_A            │
│  Pin 33 ─────┼────│────┤ CH_B            │
│              │    │    └─────────────────┘
│              │    │    ┌─────────────────┐
│              │    │    │   BL Encoder    │
│              │    ├────┤ VCC             │
│  Pin 2 ──────┼────│────┤ CH_A            │
│  Pin 35 ─────┼────│────┤ CH_B            │
│              │    │    └─────────────────┘
│              │    │    ┌─────────────────┐
│              │    │    │   BR Encoder    │
│              │    └────┤ VCC             │
│  Pin 3 ──────┼─────────┤ CH_A            │
│  Pin 37 ─────┼─────────┤ CH_B            │
│              │         └─────────────────┘
│  GND ────────┼─────────► All Encoder GNDs
└──────────────┘
```

### Encoder Specifications

| Parameter | Value |
|-----------|-------|
| CPR (Counts Per Revolution) | 360 |
| Debounce Time | 200 µs |
| Direction Multipliers | FL: +1, FR: -1, BL: +1, BR: -1 |

---

## 5. PCA9685 Servo Driver Wiring

### PCA9685 to Arduino Mega Connection

```
PCA9685 Board                    Arduino Mega 2560
┌─────────────────┐              ┌─────────────────┐
│                 │              │                 │
│  VCC ───────────┼──────────────┤  5V             │
│                 │              │                 │
│  GND ───────────┼──────────────┤  GND            │
│                 │              │                 │
│  SDA ───────────┼──────────────┤  Pin 20 (SDA)   │
│                 │              │                 │
│  SCL ───────────┼──────────────┤  Pin 21 (SCL)   │
│                 │              │                 │
│  OE  ───────────┼──────────────┤  GND (Enable)   │
│                 │              │                 │
└─────────────────┘              └─────────────────┘
        │
        │  Servo Power (SEPARATE from Arduino!)
        │
        ▼
┌─────────────────┐
│  V+  ───────────┼──────────────► External 5-6V Power (+)
│                 │
│  GND ───────────┼──────────────► External 5-6V Power (-)
└─────────────────┘
```

### PCA9685 Connection Table

| PCA9685 Pin | Connects To | Wire Color | Notes |
|-------------|-------------|------------|-------|
| **VCC** | Arduino 5V | Red | Logic power |
| **GND** | Arduino GND | Black | Common ground |
| **SDA** | Arduino Pin 20 | Blue/Yellow | I2C Data |
| **SCL** | Arduino Pin 21 | Green/White | I2C Clock |
| **OE** | GND | Black | Output Enable (active low) |
| **V+** | External 5-6V (+) | Red | Servo power |
| **GND** | External 5-6V (-) | Black | Servo power ground |

### I2C Configuration

| Parameter | Value |
|-----------|-------|
| I2C Address | 0x40 (default) |
| PWM Frequency | 50 Hz |
| PWM Range | 600 - 2400 µs |

### PCA9685 Board Layout

```
PCA9685 Board Top View:

    ┌─────────────────────────────────────────────────────────────┐
    │                                                             │
    │  ○ ○ ○ ○ ○ ○    Address Jumpers (A0-A5) - Leave open       │
    │                 for default address 0x40                    │
    │                                                             │
    │  SERVO OUTPUT HEADERS (3 pins each: PWM, V+, GND)          │
    │  ┌───┬───┬───┬───┬───┬───┬───┬───┐                         │
    │  │CH0│CH1│CH2│CH3│CH4│CH5│CH6│CH7│                         │
    │  │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ◄── PWM (Signal)       │
    │  │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ◄── V+ (Power)         │
    │  │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ◄── GND (Ground)       │
    │  └───┴───┴───┴───┴───┴───┴───┴───┘                         │
    │  BASE SHLD     WRSP WRSR     ELBW                          │
    │                                                             │
    │  ┌───┬───┬───┬───┬───┬───┬───┬───┐                         │
    │  │CH8│CH9│C10│C11│C12│C13│C14│C15│                         │
    │  │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │                         │
    │  │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │                         │
    │  │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │ ○ │                         │
    │  └───┴───┴───┴───┴───┴───┴───┴───┘                         │
    │                                GRIP                         │
    │                                                             │
    │  POWER INPUT              I2C INTERFACE                     │
    │  ┌───────────────┐        ┌─────────────────────────┐      │
    │  │  V+      GND  │        │ VCC GND SDA SCL OE      │      │
    │  │  ○        ○   │        │  ○   ○   ○   ○   ○      │      │
    │  └───────────────┘        └─────────────────────────┘      │
    │   (Servo Power)            (To Arduino)                     │
    │                                                             │
    └─────────────────────────────────────────────────────────────┘
```

---

## 6. Arm Servo Connections

### Servo Channel Assignment

| PCA9685 Channel | Joint Name | Arm Function | Home Angle | Range |
|-----------------|------------|--------------|------------|-------|
| **CH 0** | joint_1 | Base Rotation | 90° | 0-180° |
| **CH 1** | joint_2 | Shoulder | 90° | 0-180° |
| **CH 2** | - | *Unused* | - | - |
| **CH 3** | joint_4 | Wrist Pitch | 90° | 0-180° |
| **CH 4** | joint_5 | Wrist Roll | 90° | 0-180° |
| **CH 5** | - | *Unused* | - | - |
| **CH 6** | joint_3 | Elbow | 90° | 0-180° |
| **CH 7-14** | - | *Unused* | - | - |
| **CH 15** | joint_6 | Gripper | 90° | 0-180° |

### Servo Wire Colors

```
Standard Servo Wire Colors:

┌─────────────────────────────────┐
│         SERVO MOTOR             │
│                                 │
│  ┌─────────────────────────┐   │
│  │ Brown/Black ─► GND      │   │
│  │ Red ─────────► V+ (5-6V)│   │
│  │ Orange/Yellow► PWM      │   │
│  └─────────────────────────┘   │
│                                 │
└─────────────────────────────────┘
```

### Individual Servo Wiring

```
PCA9685 to Servo Connection (for each servo):

PCA9685 Channel              Servo
┌─────────────┐         ┌─────────────┐
│   PWM  ─────┼─────────┤ Signal (Org)│
│   V+   ─────┼─────────┤ Power (Red) │
│   GND  ─────┼─────────┤ Ground (Brn)│
└─────────────┘         └─────────────┘
```

### Complete Arm Wiring

```
PCA9685                              6-DOF ARM SERVOS
┌──────────────────┐
│                  │
│  CH0  ───────────┼─────────────────► BASE SERVO (Rotation)
│  (PWM,V+,GND)    │                   MG996R or similar
│                  │
│  CH1  ───────────┼─────────────────► SHOULDER SERVO
│  (PWM,V+,GND)    │                   MG996R or similar
│                  │
│  CH3  ───────────┼─────────────────► WRIST PITCH SERVO
│  (PWM,V+,GND)    │                   MG996R or similar
│                  │
│  CH4  ───────────┼─────────────────► WRIST ROLL SERVO
│  (PWM,V+,GND)    │                   MG996R or similar
│                  │
│  CH6  ───────────┼─────────────────► ELBOW SERVO
│  (PWM,V+,GND)    │                   MG996R or similar
│                  │
│  CH15 ───────────┼─────────────────► GRIPPER SERVO
│  (PWM,V+,GND)    │                   SG90 or similar
│                  │
└──────────────────┘
```

### Code Reference (Channel Array)

```cpp
// In Arduino code:
const uint8_t SERVO_CH[6] = {0, 1, 6, 3, 4, 15};
// Index mapping:
//   [0] = CH0  → Base
//   [1] = CH1  → Shoulder
//   [2] = CH6  → Elbow
//   [3] = CH3  → Wrist Pitch
//   [4] = CH4  → Wrist Roll
//   [5] = CH15 → Gripper
```

---

## 7. Raspberry Pi USB Connections

### USB Device Mapping

| Device | USB Port | Device Path | Baudrate | Chip |
|--------|----------|-------------|----------|------|
| **Arduino Mega** | USB-B | `/dev/ttyACM0` | 115200 | ATmega2560 |
| **YDLiDAR X2L** | Micro-USB | `/dev/ttyUSB0` | 115200 | CP210x |

### Connection Diagram

```
Raspberry Pi 4 USB Ports:

┌────────────────────────────────────────┐
│  RASPBERRY PI 4                        │
│                                        │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐  │
│  │USB 2 │ │USB 2 │ │USB 3 │ │USB 3 │  │
│  │      │ │      │ │(Blue)│ │(Blue)│  │
│  └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘  │
│     │        │        │        │       │
└─────┼────────┼────────┼────────┼───────┘
      │        │        │        │
      │        │        │        └──► (Available)
      │        │        │
      │        │        └──────────────► Arduino Mega
      │        │                         /dev/ttyACM0
      │        │
      │        └───────────────────────► YDLiDAR X2L
      │                                  /dev/ttyUSB0
      │
      └────────────────────────────────► (Available)
```

### Verify USB Connections

```bash
# Check connected USB devices
lsusb

# Check serial ports
ls -la /dev/ttyUSB* /dev/ttyACM*

# Expected output:
# /dev/ttyUSB0 - YDLiDAR (CP210x)
# /dev/ttyACM0 - Arduino (ACM)

# Test Arduino connection
stty -F /dev/ttyACM0 115200 raw -echo -hupcl
echo "STATUS" > /dev/ttyACM0
timeout 3 cat /dev/ttyACM0
```

---

## 8. Power Distribution

### Power Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         POWER DISTRIBUTION                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   ┌─────────────┐                                                       │
│   │  12V LIPO   │                                                       │
│   │  BATTERY    │                                                       │
│   │  (3S/4S)    │                                                       │
│   └──────┬──────┘                                                       │
│          │                                                              │
│          ├────────────────┬────────────────┬───────────────────┐       │
│          │                │                │                   │       │
│          ▼                ▼                ▼                   ▼       │
│   ┌────────────┐   ┌────────────┐   ┌────────────┐    ┌────────────┐  │
│   │  L298N #1  │   │  L298N #2  │   │ 5V Buck    │    │ 5-6V Buck  │  │
│   │  12V Input │   │  12V Input │   │ Converter  │    │ Converter  │  │
│   │            │   │            │   │ (3A min)   │    │ (5A min)   │  │
│   └─────┬──────┘   └─────┬──────┘   └─────┬──────┘    └─────┬──────┘  │
│         │                │                │                  │        │
│         ▼                ▼                ▼                  ▼        │
│   ┌──────────┐    ┌──────────┐    ┌────────────┐     ┌────────────┐  │
│   │ FL + FR  │    │ BL + BR  │    │ Raspberry  │     │  PCA9685   │  │
│   │ Motors   │    │ Motors   │    │  Pi 4      │     │  V+ Rail   │  │
│   └──────────┘    └──────────┘    │ + Arduino  │     │ (Servos)   │  │
│                                   └────────────┘     └────────────┘  │
│                                                                       │
│   COMMON GROUND: All GNDs must be connected together!                │
│                                                                       │
└─────────────────────────────────────────────────────────────────────────┘
```

### Power Requirements

| Component | Voltage | Current (Peak) | Notes |
|-----------|---------|----------------|-------|
| **Raspberry Pi 4** | 5V | 3A | USB-C power |
| **Arduino Mega** | 5V (USB) | 500mA | Powered via RPi USB |
| **4x DC Motors** | 12V | 2A each (8A total) | Via L298N |
| **6x Servos** | 5-6V | 1A each (6A total) | Via PCA9685 V+ |
| **YDLiDAR X2L** | 5V | 500mA | Via USB |

### Power Wiring Diagram

```
                    12V Battery
                         │
          ┌──────────────┼──────────────┐
          │              │              │
          ▼              ▼              ▼
     ┌─────────┐    ┌─────────┐    ┌─────────┐
     │ L298N#1 │    │ L298N#2 │    │  5V DC  │
     │  12V    │    │  12V    │    │  Buck   │
     └────┬────┘    └────┬────┘    └────┬────┘
          │              │              │
          ▼              ▼              ├──────► RPi 5V
     FL+FR Motors   BL+BR Motors       │
                                       └──────► Arduino (via USB)

                    12V Battery
                         │
                         ▼
                   ┌─────────┐
                   │ 5-6V DC │
                   │  Buck   │
                   │ (5A)    │
                   └────┬────┘
                        │
                        ▼
                   PCA9685 V+ ──────► All 6 Servos
```

### Important Power Notes

1. **NEVER power servos from Arduino 5V** - servos draw too much current
2. **Use separate buck converter for servos** - minimum 5A capacity
3. **Common ground is critical** - connect all GNDs together
4. **Battery voltage** - 11.1V (3S) or 14.8V (4S) LiPo recommended

---

## 9. Complete Wiring Diagram

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                              COMPLETE SYSTEM WIRING                                      │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│                              ┌─────────────────────────────────────────┐                │
│                              │         ARDUINO MEGA 2560              │                │
│                              │                                         │                │
│  ┌──────────┐                │  MOTOR CONTROL (L298N)                  │                │
│  │ YDLiDAR  │     USB        │  ┌─────────────────────────────────┐   │                │
│  │   X2L    ├────────────┐   │  │  Pin 22,23,12 ─► FL Motor      │   │                │
│  └──────────┘            │   │  │  Pin 24,25,11 ─► FR Motor      │   │                │
│                          │   │  │  Pin 26,27,10 ─► BL Motor      │   │                │
│  ┌──────────┐            │   │  │  Pin 28,29,9  ─► BR Motor      │   │                │
│  │Raspberry │◄───────────┤   │  └─────────────────────────────────┘   │                │
│  │  Pi 4    │  USB       │   │                                         │                │
│  │          │◄───────────┼───┤  ENCODERS                               │                │
│  └──────────┘ /ttyACM0   │   │  ┌─────────────────────────────────┐   │                │
│                          │   │  │  Pin 18,31 ─► FL Encoder       │   │                │
│                          │   │  │  Pin 19,33 ─► FR Encoder       │   │                │
│   ┌─────────────────┐    │   │  │  Pin 2,35  ─► BL Encoder       │   │                │
│   │   L298N #1      │◄───┼───┤  │  Pin 3,37  ─► BR Encoder       │   │                │
│   │  FL + FR Motors │    │   │  └─────────────────────────────────┘   │                │
│   └─────────────────┘    │   │                                         │                │
│                          │   │  I2C (PCA9685)                          │                │
│   ┌─────────────────┐    │   │  ┌─────────────────────────────────┐   │                │
│   │   L298N #2      │◄───┼───┤  │  Pin 20 (SDA) ─┐                │   │                │
│   │  BL + BR Motors │    │   │  │  Pin 21 (SCL) ─┼─► PCA9685      │   │                │
│   └─────────────────┘    │   │  │                │    │           │   │                │
│                          │   │  └────────────────┼────┼───────────┘   │                │
│   ┌─────────────────┐    │   │                   │    │               │                │
│   │   4x Encoders   │◄───┼───┤                   │    │               │                │
│   │ FL,FR,BL,BR     │    │   └───────────────────┼────┼───────────────┘                │
│   └─────────────────┘    │                       │    │                                │
│                          │                       ▼    ▼                                │
│                          │               ┌─────────────────┐                           │
│                          │               │    PCA9685      │                           │
│                          │               │    I2C: 0x40    │                           │
│                          │               │                 │                           │
│                          │               │  CH0  ─► Base Servo                         │
│                          │               │  CH1  ─► Shoulder Servo                     │
│                          │               │  CH3  ─► Wrist Pitch Servo                  │
│                          │               │  CH4  ─► Wrist Roll Servo                   │
│                          │               │  CH6  ─► Elbow Servo                        │
│                          │               │  CH15 ─► Gripper Servo                      │
│                          │               │                 │                           │
│                          │               │  V+ ◄── 5-6V Servo Power                    │
│                          │               └─────────────────┘                           │
│                          │                                                             │
│                          └──► /dev/ttyUSB0                                             │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Quick Reference Tables

### All Arduino Pin Assignments

| Pin | Function | Component | Notes |
|-----|----------|-----------|-------|
| 2 | INT0 | BL Encoder A | Interrupt |
| 3 | INT1 | BR Encoder A | Interrupt |
| 9 | PWM | BR Motor EN | Speed |
| 10 | PWM | BL Motor EN | Speed |
| 11 | PWM | FR Motor EN | Speed |
| 12 | PWM | FL Motor EN | Speed |
| 18 | INT3 | FL Encoder A | Interrupt |
| 19 | INT4 | FR Encoder A | Interrupt |
| 20 | SDA | PCA9685 | I2C |
| 21 | SCL | PCA9685 | I2C |
| 22 | Digital | FL Motor IN1 | Direction |
| 23 | Digital | FL Motor IN2 | Direction |
| 24 | Digital | FR Motor IN1 | Direction |
| 25 | Digital | FR Motor IN2 | Direction |
| 26 | Digital | BL Motor IN1 | Direction |
| 27 | Digital | BL Motor IN2 | Direction |
| 28 | Digital | BR Motor IN1 | Direction |
| 29 | Digital | BR Motor IN2 | Direction |
| 31 | Digital | FL Encoder B | Direction |
| 33 | Digital | FR Encoder B | Direction |
| 35 | Digital | BL Encoder B | Direction |
| 37 | Digital | BR Encoder B | Direction |

### PCA9685 Channel Assignments

| Channel | Joint | Function |
|---------|-------|----------|
| CH0 | joint_1 | Base |
| CH1 | joint_2 | Shoulder |
| CH3 | joint_4 | Wrist Pitch |
| CH4 | joint_5 | Wrist Roll |
| CH6 | joint_3 | Elbow |
| CH15 | joint_6 | Gripper |

### Serial Connections

| Device | Port | Baudrate |
|--------|------|----------|
| Arduino Mega | /dev/ttyACM0 | 115200 |
| YDLiDAR X2L | /dev/ttyUSB0 | 115200 |

### Robot Parameters

| Parameter | Value |
|-----------|-------|
| Wheel Radius | 0.075 m |
| Wheel Base | 0.35 m |
| Encoder CPR | 360 |
| Servo PWM Min | 600 µs |
| Servo PWM Max | 2400 µs |
| Servo Frequency | 50 Hz |

---

## 11. Troubleshooting

### Arduino Not Detected

```bash
# Check USB connection
lsusb | grep Arduino

# Check device
ls /dev/ttyACM*

# Fix permissions
sudo chmod 666 /dev/ttyACM0
sudo usermod -a -G dialout $USER
# (Log out and back in after usermod)
```

### PCA9685 Not Found

```bash
# Check I2C devices (on RPi or via Arduino)
i2cdetect -y 1

# Should show device at address 0x40
```

Arduino code will show:
```
STATUS,PCA9685 not found - Arm disabled
```

**Check:**
1. SDA connected to Pin 20
2. SCL connected to Pin 21
3. VCC connected to 5V
4. GND connected
5. I2C address jumpers (should all be open for 0x40)

### Motors Not Moving

1. Check L298N 12V power connection
2. Verify IN1/IN2/EN wiring
3. Test with direct serial command:
   ```
   echo "VEL,0.1,0" > /dev/ttyACM0
   ```
4. Send unlock command:
   ```
   echo "UNLOCK,ALL" > /dev/ttyACM0
   ```

### Servos Not Moving

1. Check PCA9685 V+ power (separate 5-6V supply)
2. Verify servo signal wire to correct channel
3. Test with direct command:
   ```
   echo "ARM,90,45,90,90,90,90" > /dev/ttyACM0
   ```

### LiDAR Not Detected

```bash
# Check USB
lsusb | grep CP210

# Check device
ls /dev/ttyUSB*

# Fix permissions
sudo chmod 666 /dev/ttyUSB0
```

---

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-23 | Initial complete wiring guide |

---

*Generated for Mobile Manipulator Robot Project*
