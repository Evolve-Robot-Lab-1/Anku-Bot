# ROS2 Mobile Manipulator Playbook
**Project: Gripper Car - 4WD Base + 6-DOF Arm + LiDAR**
**Last Updated: 2025-12-29**

---

## Table of Contents
1. [What is ROS2?](#1-what-is-ros2)
2. [Core Concepts](#2-core-concepts)
3. [System Architecture](#3-system-architecture)
4. [Packages & Nodes](#4-packages--nodes)
5. [Data Flow Diagrams](#5-data-flow-diagrams)
6. [Topic Map](#6-topic-map)
7. [File Locations](#7-file-locations)
8. [Hardware Connections](#8-hardware-connections)
9. [Quick Start Commands](#9-quick-start-commands)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. What is ROS2?

### The Core Problem ROS Solves

Imagine building a robot with:
- 4 motors for wheels
- 6 servos for an arm
- A LiDAR sensor
- A camera
- A brain (computer) to control everything

**Without ROS:** You'd have to write ONE giant program that:
- Reads the LiDAR
- Processes camera images
- Calculates motor speeds
- Moves servos
- All at once, all tangled together

This becomes a nightmare to debug, modify, or scale.

### What ROS Does: "Divide and Conquer"

**ROS splits your robot into small, independent programs called NODES.**

Each node does ONE job:
```
Node 1: "I only read the LiDAR and share the data"
Node 2: "I only listen for velocity commands and drive motors"
Node 3: "I only plan arm movements"
Node 4: "I only detect QR codes from camera"
```

### The "First Principle" Summary

```
1. HARDWARE does physical things (motors spin, laser scans)

2. ARDUINO translates simple text commands → hardware signals
   (No ROS, just serial protocol)

3. NODES are programs that do ONE thing well
   (read sensor, drive motor, plan path)

4. TOPICS are named channels for data
   ("/cmd_vel" = velocity commands, "/scan" = laser data)

5. PACKAGES group related nodes together
   (base_controller has all base-related nodes)

6. PUBLISHERS send data to topics
   (teleop_keyboard publishes to /cmd_vel)

7. SUBSCRIBERS receive data from topics
   (serial_bridge subscribes to /cmd_vel)

8. NETWORK lets nodes run on different computers
   (PC and RPi share the same topic space via WiFi)
```

---

## 2. Core Concepts

### 2.1 Nodes
A **Node** is a single program that does one specific task.

```
Examples:
- teleop_keyboard node: Reads keyboard, publishes velocity
- serial_bridge node: Bridges ROS ↔ Arduino
- ydlidar_driver node: Reads LiDAR, publishes scan data
```

### 2.2 Topics
A **Topic** is a named channel where nodes send/receive data.

```
┌─────────────┐                      ┌─────────────┐
│  Keyboard   │ ──publishes───►      │   Motors    │
│   Node      │    /cmd_vel          │    Node     │
│             │   "go forward"       │             │
│  PUBLISHER  │                      │ SUBSCRIBER  │
└─────────────┘                      └─────────────┘
```

- **Publisher**: Broadcasts data to a topic (like a radio station)
- **Subscriber**: Listens to a topic (like tuning into a station)
- **Topic**: The channel name (e.g., `/cmd_vel`, `/scan`)

**Key insight:** The keyboard node doesn't know WHO is listening. The motor node doesn't know WHO is sending. They just agree on the channel name.

### 2.3 Packages
A **Package** is a folder containing related code:

```
base_controller/              ← PACKAGE
├── package.xml               ← "I am a ROS package named base_controller"
├── scripts/
│   ├── serial_bridge_node.py ← NODE (the actual program)
│   └── odom_publisher_node.py← Another NODE
├── launch/
│   └── hardware.launch.py    ← Starts multiple nodes at once
└── config/
    └── base_params.yaml      ← Parameters for nodes
```

### 2.4 Messages
**Messages** are the data structures sent over topics:

```
geometry_msgs/msg/Twist:
  linear:
    x: 0.3    # forward speed (m/s)
    y: 0.0
    z: 0.0
  angular:
    x: 0.0
    y: 0.0
    z: 0.5    # rotation speed (rad/s)

sensor_msgs/msg/LaserScan:
  ranges: [0.5, 0.6, 0.7, ...]  # distances at each angle
  angle_min: -3.14
  angle_max: 3.14
```

### 2.5 Why Two Computers? (PC + RPi)

| Computer | Why? |
|----------|------|
| **RPi** | Small, sits on robot, connects to hardware (Arduino, LiDAR) |
| **PC** | Powerful, runs heavy computation (MoveIt, RViz, path planning) |

**ROS magic:** Both computers share the same "topic space" over WiFi.

```
PC publishes /cmd_vel  ──────WiFi──────►  RPi subscribes /cmd_vel
                         (same topic!)
```

This requires matching environment variables:
```bash
export ROS_DOMAIN_ID=0              # Same "room" for all nodes
export ROS_LOCALHOST_ONLY=0         # Allow network communication
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp  # Same "language"
```

---

## 3. System Architecture

### 3.1 Overview Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              PC (Your Computer)                              │
│                                                                              │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────────────┐   │
│  │ teleop_keyboard  │  │     MoveIt       │  │         RViz             │   │
│  │                  │  │   (move_group)   │  │    (visualization)       │   │
│  │ Publishes:       │  │                  │  │                          │   │
│  │  /cmd_vel        │  │ Publishes:       │  │ Subscribes:              │   │
│  └────────┬─────────┘  │  /arm/joint_cmds │  │  /scan, /tf, /odom       │   │
│           │            └────────┬─────────┘  └──────────────────────────┘   │
│           │                     │                                            │
└───────────┼─────────────────────┼────────────────────────────────────────────┘
            │                     │
            │    WiFi Network     │     (ROS_DOMAIN_ID=0, RMW=fastrtps)
            │    192.168.1.x      │
            ▼                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          RPi (192.168.1.2)                                   │
│                                                                              │
│  ┌────────────────────────────────┐    ┌─────────────────────────────────┐  │
│  │     serial_bridge_node.py      │    │   ydlidar_ros2_driver_node     │  │
│  │     (base_controller pkg)      │    │   (ydlidar_ros2_driver pkg)    │  │
│  │                                │    │                                 │  │
│  │ Subscribes:                    │    │ Connects to:                    │  │
│  │   /cmd_vel → VEL command       │    │   /dev/ttyAMA0 (GPIO UART)     │  │
│  │   /arm/joint_commands → SERVO  │    │                                 │  │
│  │                                │    │ Publishes:                      │  │
│  │ Publishes:                     │    │   /scan (LaserScan)            │  │
│  │   /encoder_ticks               │    │                                 │  │
│  │   /arm/joint_states            │    │ NO Arduino needed!              │  │
│  │   /arduino_ack                 │    │ LiDAR talks directly to RPi    │  │
│  └────────────┬───────────────────┘    └─────────────────────────────────┘  │
│               │                                                              │
│               │ Serial: /dev/ttyACM0                                        │
│               │ 115200 baud                                                  │
└───────────────┼──────────────────────────────────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Arduino Mega 2560                                     │
│                    (mobile_manipulator_unified.ino)                          │
│                                                                              │
│  Receives commands via Serial:          Sends data via Serial:              │
│  ┌─────────────────────────────┐        ┌─────────────────────────────┐     │
│  │ VEL,<linear>,<angular>      │        │ ENC,<fl>,<fr>,<bl>,<br>     │     │
│  │   → Controls 4 DC motors    │        │   → Encoder counts          │     │
│  │                             │        │                             │     │
│  │ SET_ALL_SERVOS,a0,a1,...,a5 │        │ OK,<command>                │     │
│  │   → Controls 6 servos       │        │   → Acknowledgment          │     │
│  │     via PCA9685             │        │                             │     │
│  │                             │        │ STATUS,<message>            │     │
│  │ STOP                        │        │   → Status updates          │     │
│  │   → Stops all motors        │        │                             │     │
│  └─────────────────────────────┘        └─────────────────────────────┘     │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                           LiDAR (YDLIDAR X2)                                 │
│                                                                              │
│  Connected directly to RPi GPIO UART (/dev/ttyAMA0)                         │
│  NOT through Arduino!                                                        │
│                                                                              │
│  Wiring:                                                                     │
│  ├─ TX  → RPi RX (GPIO 15)                                                  │
│  ├─ RX  → RPi TX (GPIO 14)                                                  │
│  ├─ GND → RPi GND                                                           │
│  └─ VCC → 5V                                                                │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Packages & Nodes

### 4.1 On RPi (Hardware Interface)

| Package | Node | What it does |
|---------|------|--------------|
| `base_controller` | `serial_bridge_node.py` | Talks to Arduino (base + arm) |
| `base_controller` | `odom_publisher_node.py` | Calculates robot position from encoders |
| `ydlidar_ros2_driver` | `ydlidar_ros2_driver_node` | Reads LiDAR, publishes `/scan` |

### 4.2 On PC (Control & Visualization)

| Package | Node | What it does |
|---------|------|--------------|
| `teleop_twist_keyboard` | `teleop_twist_keyboard` | Keyboard → `/cmd_vel` |
| `arm_controller` | `servo_bridge.py` | (Alternative) Direct arm control |
| `arm_controller` | `trajectory_bridge.py` | MoveIt → `/arm/joint_commands` |
| `arm_moveit` | `move_group` | Motion planning for arm |
| `robot_state_publisher` | `robot_state_publisher` | URDF → TF transforms |

---

## 5. Data Flow Diagrams

### 5.1 Base Control Flow (Moving the Robot)

```
┌────────────────────────────────────────────────────────────────────────────┐
│                                  PC                                         │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  PACKAGE: teleop_twist_keyboard (system package)                     │   │
│  │  NODE: teleop_twist_keyboard                                         │   │
│  │                                                                       │   │
│  │  You press 'i' key                                                   │   │
│  │       ↓                                                               │   │
│  │  PUBLISHES to topic: /cmd_vel                                        │   │
│  │  Message type: geometry_msgs/msg/Twist                               │   │
│  │  Data: {linear: {x: 0.3, y: 0, z: 0}, angular: {x: 0, y: 0, z: 0}}  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    │ /cmd_vel                               │
│                                    ▼                                        │
└────────────────────────────────────┼────────────────────────────────────────┘
                                     │
                              ~~~~ WiFi ~~~~
                              (ROS_DOMAIN_ID=0)
                                     │
                                     ▼
┌────────────────────────────────────┼────────────────────────────────────────┐
│                                   RPi                                        │
│                                    │                                         │
│  ┌─────────────────────────────────▼───────────────────────────────────┐    │
│  │  PACKAGE: base_controller                                            │    │
│  │  NODE: serial_bridge_node.py                                         │    │
│  │                                                                       │    │
│  │  SUBSCRIBES to: /cmd_vel                                             │    │
│  │       ↓                                                               │    │
│  │  Receives: {linear: {x: 0.3}, angular: {z: 0}}                       │    │
│  │       ↓                                                               │    │
│  │  Converts to string: "VEL,0.3000,0.0000\n"                           │    │
│  │       ↓                                                               │    │
│  │  Writes to serial port: /dev/ttyACM0                                 │    │
│  └─────────────────────────────────┬───────────────────────────────────┘    │
│                                    │                                         │
│                                    │ USB Serial Cable                        │
│                                    ▼                                         │
└────────────────────────────────────┼─────────────────────────────────────────┘
                                     │
                                     ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                             ARDUINO MEGA                                    │
│                   (NOT ROS - just C++ firmware)                             │
│                                                                             │
│  FILE: mobile_manipulator_unified.ino                                       │
│                                                                             │
│  Serial.read() receives: "VEL,0.3000,0.0000\n"                             │
│       ↓                                                                     │
│  Parses: linear_vel = 0.3, angular_vel = 0.0                               │
│       ↓                                                                     │
│  Calculates wheel speeds (differential drive):                              │
│     left_speed  = linear - (angular * wheel_base / 2)                      │
│     right_speed = linear + (angular * wheel_base / 2)                      │
│       ↓                                                                     │
│  Sets PWM on motor pins:                                                    │
│     Pin 12 (FL), Pin 11 (FR), Pin 10 (BL), Pin 9 (BR)                      │
│       ↓                                                                     │
│  ROBOT WHEELS SPIN!                                                         │
└────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 Arm Control Flow (Moving the Arm)

```
┌────────────────────────────────────────────────────────────────────────────┐
│                                  PC                                         │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  PACKAGE: arm_moveit                                                 │   │
│  │  NODE: move_group (MoveIt2)                                          │   │
│  │                                                                       │   │
│  │  You drag arm in RViz                                                │   │
│  │       ↓                                                               │   │
│  │  MoveIt calculates trajectory (list of joint positions over time)   │   │
│  │       ↓                                                               │   │
│  │  PUBLISHES: /arm_controller/follow_joint_trajectory (action)        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  PACKAGE: arm_controller                                             │   │
│  │  NODE: trajectory_bridge.py                                          │   │
│  │                                                                       │   │
│  │  SUBSCRIBES to: MoveIt trajectory                                    │   │
│  │       ↓                                                               │   │
│  │  Extracts joint positions: [0.5, -0.3, 0.8, 0.0, 0.0, 0.2]          │   │
│  │       ↓                                                               │   │
│  │  PUBLISHES to: /arm/joint_commands                                   │   │
│  │  Message type: sensor_msgs/msg/JointState                            │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    │ /arm/joint_commands                    │
│                                    ▼                                        │
└────────────────────────────────────┼────────────────────────────────────────┘
                                     │
                              ~~~~ WiFi ~~~~
                                     │
                                     ▼
┌────────────────────────────────────┼────────────────────────────────────────┐
│                                   RPi                                        │
│                                    │                                         │
│  ┌─────────────────────────────────▼───────────────────────────────────┐    │
│  │  PACKAGE: base_controller                                            │    │
│  │  NODE: serial_bridge_node.py                                         │    │
│  │                                                                       │    │
│  │  SUBSCRIBES to: /arm/joint_commands                                  │    │
│  │       ↓                                                               │    │
│  │  Receives: {name: [joint_1,...], position: [0.5, -0.3, 0.8,...]}    │    │
│  │       ↓                                                               │    │
│  │  Converts radians → servo degrees (0-180):                           │    │
│  │     joint_to_servo_angle() function                                  │    │
│  │       ↓                                                               │    │
│  │  Creates string: "SET_ALL_SERVOS,120,75,140,90,90,100\n"            │    │
│  │       ↓                                                               │    │
│  │  Writes to serial: /dev/ttyACM0                                      │    │
│  └─────────────────────────────────┬───────────────────────────────────┘    │
│                                    │                                         │
└────────────────────────────────────┼─────────────────────────────────────────┘
                                     │ USB Serial
                                     ▼
┌────────────────────────────────────────────────────────────────────────────┐
│                             ARDUINO MEGA                                    │
│                                                                             │
│  Serial.read() receives: "SET_ALL_SERVOS,120,75,140,90,90,100\n"           │
│       ↓                                                                     │
│  Parses angles: servo[0]=120, servo[1]=75, ...                             │
│       ↓                                                                     │
│  Sends to PCA9685 via I2C:                                                 │
│     pca.setPWM(channel, 0, angleToPulse(angle))                            │
│       ↓                                                                     │
│  ARM SERVOS MOVE!                                                           │
└────────────────────────────────────────────────────────────────────────────┘
```

### 5.3 LiDAR Flow (Laser Scanning)

```
┌────────────────────────────────────────────────────────────────────────────┐
│                           YDLIDAR X2 (Hardware)                             │
│                                                                             │
│  - Motor spins the laser                                                    │
│  - Measures distance at each angle                                          │
│  - Sends raw data via serial TX pin                                        │
│       ↓                                                                     │
│  Wires: TX→RPi RX, RX→RPi TX, GND, 5V                                      │
└────────────────────────────────────┬───────────────────────────────────────┘
                                     │ GPIO UART /dev/ttyAMA0
                                     ▼
┌────────────────────────────────────┼────────────────────────────────────────┐
│                                   RPi                                        │
│                                    │                                         │
│  ┌─────────────────────────────────▼───────────────────────────────────┐    │
│  │  PACKAGE: ydlidar_ros2_driver                                        │    │
│  │  NODE: ydlidar_ros2_driver_node                                      │    │
│  │                                                                       │    │
│  │  Opens serial port: /dev/ttyAMA0 @ 115200 baud                       │    │
│  │       ↓                                                               │    │
│  │  Reads raw bytes from LiDAR                                          │    │
│  │       ↓                                                               │    │
│  │  Decodes into: angle[], distance[] arrays                            │    │
│  │       ↓                                                               │    │
│  │  PUBLISHES to: /scan                                                 │    │
│  │  Message type: sensor_msgs/msg/LaserScan                             │    │
│  │  Data: {ranges: [0.5, 0.6, 0.7, ...], angle_min: -3.14, ...}        │    │
│  └─────────────────────────────────┬───────────────────────────────────┘    │
│                                    │                                         │
│                                    │ /scan                                   │
│                                    ▼                                         │
└────────────────────────────────────┼─────────────────────────────────────────┘
                                     │
                              ~~~~ WiFi ~~~~
                                     │
                                     ▼
┌────────────────────────────────────┼────────────────────────────────────────┐
│                                   PC                                         │
│                                    │                                         │
│  ┌─────────────────────────────────▼───────────────────────────────────┐    │
│  │  PACKAGE: rviz2                                                       │    │
│  │  NODE: rviz2                                                          │    │
│  │                                                                       │    │
│  │  SUBSCRIBES to: /scan                                                 │    │
│  │       ↓                                                               │    │
│  │  Displays laser points as red dots around robot                       │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  PACKAGE: nav2 (navigation)                                           │    │
│  │  NODE: slam_toolbox / nav2_controller                                 │    │
│  │                                                                       │    │
│  │  SUBSCRIBES to: /scan                                                 │    │
│  │       ↓                                                               │    │
│  │  Uses for: obstacle avoidance, mapping, localization                  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Topic Map

```
                          YOUR ROBOT'S TOPICS

┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│   TOPIC NAME              TYPE                    PUBLISHER → SUBSCRIBER    │
│   ══════════              ════                    ══════════════════════    │
│                                                                             │
│   /cmd_vel                Twist                   teleop_keyboard           │
│                                                      → serial_bridge        │
│                                                                             │
│   /arm/joint_commands     JointState              trajectory_bridge         │
│                                                      → serial_bridge        │
│                                                                             │
│   /arm/joint_states       JointState              serial_bridge             │
│                                                      → MoveIt, RViz         │
│                                                                             │
│   /encoder_ticks          Int64MultiArray         serial_bridge             │
│                                                      → odom_publisher       │
│                                                                             │
│   /odom                   Odometry                odom_publisher            │
│                                                      → Nav2, RViz           │
│                                                                             │
│   /scan                   LaserScan               ydlidar_driver            │
│                                                      → RViz, Nav2, SLAM     │
│                                                                             │
│   /tf                     TFMessage               robot_state_publisher     │
│                                                      → Everyone             │
│                                                                             │
│   /joint_states           JointState              (aggregated)              │
│                                                      → robot_state_pub      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 7. File Locations

```
gripper_car_ws_v1/src/
│
├── mobile_base/
│   └── base_controller/
│       └── scripts/
│           ├── serial_bridge_node.py    ← MAIN BRIDGE (base + arm)
│           ├── odom_publisher_node.py   ← Encoders → Odometry
│           └── teleop_keyboard.py       ← Keyboard control
│
├── arm/
│   ├── arm_controller/
│   │   └── scripts/
│   │       ├── servo_bridge.py          ← Direct arm control
│   │       └── trajectory_bridge.py     ← MoveIt → JointState
│   │
│   └── arm_moveit/                      ← MoveIt configuration
│       ├── config/
│       └── launch/demo.launch.py
│
├── ydlidar_ros2_driver/
│   ├── src/ydlidar_ros2_driver_node.cpp ← LiDAR driver
│   └── params/X2.yaml                   ← LiDAR config
│
└── arduino/
    └── mobile_manipulator_unified/
        └── mobile_manipulator_unified.ino  ← Arduino firmware
```

---

## 8. Hardware Connections

### 8.1 Network Setup

| Device | IP Address | Credentials |
|--------|------------|-------------|
| **RPi** | 192.168.1.2 | user: `pi`, password: `123` |
| **PC** | 192.168.1.x | (same network) |

### 8.2 Motor Pin Configuration (Arduino)

| Motor | EN (PWM) | IN1 | IN2 |
|-------|----------|-----|-----|
| Front Left | 12 | 22 | 23 |
| Front Right | 11 | 24 | 25 |
| Back Left | 10 | 26 | 27 |
| Back Right | 9 | 28 | 29 |

### 8.3 Arm Servo Configuration (PCA9685)

| Servo | Joint | Channel |
|-------|-------|---------|
| 0 | Base rotation (joint_1) | 0 |
| 1 | Shoulder (joint_2) | 1 |
| 2 | Elbow (joint_3) | 2 |
| 3 | Wrist (joint_4) | 3 |
| 4 | Gripper rotation (gripper_base_joint) | 4 |
| 5 | Gripper open/close (left_gear_joint) | 5 |

### 8.4 LiDAR Wiring (GPIO UART)

| LiDAR Pin | RPi Pin |
|-----------|---------|
| TX | GPIO 15 (RX) |
| RX | GPIO 14 (TX) |
| GND | GND |
| VCC | 5V |

### 8.5 Serial Ports

| Device | Port | Baud Rate |
|--------|------|-----------|
| Arduino | /dev/ttyACM0 | 115200 |
| LiDAR | /dev/ttyAMA0 | 115200 |

---

## 9. Quick Start Commands

### 9.1 Environment Setup (Run on BOTH PC and RPi)

```bash
export ROS_DOMAIN_ID=0
export ROS_LOCALHOST_ONLY=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
source /opt/ros/humble/setup.bash
```

### 9.2 Launch Unified Bridge on RPi (via SSH)

```bash
sshpass -p '123' ssh pi@192.168.1.2 "
  pkill -9 -f python3 2>/dev/null; sleep 2
  nohup bash -c '
    source /opt/ros/humble/setup.bash && \
    source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash && \
    export ROS_DOMAIN_ID=0 && \
    export ROS_LOCALHOST_ONLY=0 && \
    export RMW_IMPLEMENTATION=rmw_fastrtps_cpp && \
    ros2 run base_controller serial_bridge_node.py
  ' > /tmp/unified_bridge.log 2>&1 &
  sleep 5
  tail -5 /tmp/unified_bridge.log
"
```

### 9.3 Control Base

```bash
# Keyboard teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Manual commands
# Forward (3 seconds)
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.3}}" -r 10

# Turn left
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{angular: {z: 0.5}}" -r 10

# Stop
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}}"
```

### 9.4 Control Arm

```bash
# Direct servo command
ros2 topic pub --once /arduino_cmd std_msgs/msg/String "{data: 'SET_ALL_SERVOS,90,90,90,90,90,90'}"

# Via MoveIt
ros2 launch arm_moveit demo.launch.py
```

### 9.5 Launch LiDAR

```bash
# On RPi
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args --params-file /home/pi/pi_ros_ws/gripper_car_ws/src/ydlidar_ros2_driver/params/X2.yaml

# Check from PC
ros2 topic echo /scan
```

### 9.6 Emergency Stop

```bash
# Method 1: ROS command
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}}"

# Method 2: Kill processes
sshpass -p '123' ssh pi@192.168.1.2 "killall -9 python3"

# Method 3: Direct serial STOP
sshpass -p '123' ssh pi@192.168.1.2 "python3 -c \"
import serial, time
s = serial.Serial('/dev/ttyACM0', 115200)
time.sleep(2.5)
s.write(b'STOP\n')
s.close()
\""
```

---

## 10. Troubleshooting

### 10.1 Can't See Topics from PC

```bash
# 1. Check both machines use same RMW
echo $RMW_IMPLEMENTATION  # Should be rmw_fastrtps_cpp

# 2. Restart ROS daemon
ros2 daemon stop && ros2 daemon start

# 3. Check network
ping 192.168.1.2
```

### 10.2 Motors Won't Stop

```bash
# 1. Kill all processes
sshpass -p '123' ssh pi@192.168.1.2 "killall -9 python3 ros2"

# 2. Wait 3 seconds (Arduino timeout)

# 3. If still running, use direct serial STOP
```

### 10.3 Serial Port Locked

```bash
sshpass -p '123' ssh pi@192.168.1.2 "fuser -k /dev/ttyACM0; killall -9 python3"
```

### 10.4 LiDAR Not Working

```bash
# Check port exists
ls -la /dev/ttyAMA0

# Check permissions
sudo chmod 666 /dev/ttyAMA0

# If still fails, try swapping TX/RX wires
```

### 10.5 MoveIt Planning Fails

```bash
# Check joint_limits.yaml has max_acceleration > 0
# Should be 5.0, not 0.0
```

---

## Arduino Serial Protocol Reference

### Commands (ROS → Arduino)

| Command | Format | Example |
|---------|--------|---------|
| Velocity | `VEL,<linear>,<angular>` | `VEL,0.25,0.0` |
| Arm | `SET_ALL_SERVOS,<a0>,...,<a5>` | `SET_ALL_SERVOS,90,90,90,90,90,90` |
| Stop All | `STOP` | `STOP` |
| Reset Encoders | `RST` | `RST` |

### Responses (Arduino → ROS)

| Response | Format |
|----------|--------|
| Encoders | `ENC,<fl>,<fr>,<bl>,<br>` |
| Acknowledgment | `OK,<command>` |
| Status | `STATUS,<message>` |
| Error | `ERROR,<message>` |

---

*End of ROS2 Playbook*
