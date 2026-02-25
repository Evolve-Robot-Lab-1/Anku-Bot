# ROS2 Mobile Manipulator Workspace

A comprehensive ROS2 Humble workspace for a mobile manipulator robot featuring a 4-wheel differential drive base with a 6-DOF robotic arm and gripper. This project is designed for learners and provides detailed documentation for each component.

## Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Hardware Requirements](#hardware-requirements)
- [Software Dependencies](#software-dependencies)
- [Workspace Structure](#workspace-structure)
- [Quick Start](#quick-start)
- [Package Documentation](#package-documentation)
- [Communication Patterns](#communication-patterns)
- [Troubleshooting](#troubleshooting)

---

## Overview

This workspace implements a modular mobile manipulator system consisting of:

- **Mobile Base**: 4-wheel differential drive platform with encoder feedback
- **Robotic Arm**: 6-DOF servo-driven arm with gripper end-effector
- **Sensors**: YDLiDAR X2L for navigation, Raspberry Pi camera for object detection
- **Applications**: Pick-and-place task with QR code detection and autonomous navigation

### Key Features

- Modular package design for easy customization
- Hardware abstraction through ROS2 bridges
- Full Gazebo simulation support
- MoveIt2 integration for motion planning
- Nav2 integration for autonomous navigation
- SLAM capabilities for mapping unknown environments

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           APPLICATION LAYER                                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │
│  │  Pick & Place   │  │   Navigation    │  │   Manipulation  │              │
│  │  State Machine  │  │   (Nav2)        │  │   (MoveIt2)     │              │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘              │
└───────────┼─────────────────────┼─────────────────────┼──────────────────────┘
            │                     │                     │
            ▼                     ▼                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ROS2 MIDDLEWARE                                    │
│  Topics: /cmd_vel, /odom, /scan, /joint_states, /tf, /goal_pose             │
│  Services: /move_group/plan, /qr_scanner/detect                             │
│  Actions: /navigate_to_pose, /follow_joint_trajectory                       │
└─────────────────────────────────────────────────────────────────────────────┘
            │                     │                     │
            ▼                     ▼                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           HARDWARE ABSTRACTION                               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │
│  │ Base Controller │  │ Arm Controller  │  │ Sensor Drivers  │              │
│  │ (motor bridge)  │  │ (servo bridge)  │  │ (LiDAR/Camera)  │              │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘              │
└───────────┼─────────────────────┼─────────────────────┼──────────────────────┘
            │                     │                     │
            ▼                     ▼                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           PHYSICAL HARDWARE                                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │
│  │   DC Motors     │  │  Servo Motors   │  │  YDLiDAR X2L    │              │
│  │   (4x wheels)   │  │  (6-DOF + grip) │  │  Pi Camera      │              │
│  │   + Encoders    │  │  via PCA9685    │  │                 │              │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘              │
│                              ▲                                               │
│                              │                                               │
│                     ┌────────┴────────┐                                      │
│                     │     Arduino     │                                      │
│                     │   Mega 2560     │                                      │
│                     └─────────────────┘                                      │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Hardware Requirements

### Compute Platform
- **Raspberry Pi 4** (4GB+ RAM recommended) - Robot onboard computer
- **Development PC** (optional) - For RViz visualization and development

### Mobile Base
- 4x DC motors with encoders
- L298N or similar motor drivers
- 4x wheels (75mm diameter recommended)

### Robotic Arm
- 6x servo motors (MG996R or similar)
- PCA9685 PWM driver board
- Gripper servo (SG90 or similar)

### Sensors
- YDLiDAR X2L (or compatible X2/X4 model)
- Raspberry Pi Camera v2 (or USB webcam)

### Controller
- Arduino Mega 2560 (for motor/servo control)

### Power
- 12V battery for motors
- 5V/6V regulated supply for servos
- 5V supply for Raspberry Pi

---

## Software Dependencies

### ROS2 Packages
```bash
# Core dependencies
sudo apt install ros-humble-ros-base

# Navigation
sudo apt install ros-humble-navigation2 ros-humble-nav2-bringup

# MoveIt2
sudo apt install ros-humble-moveit ros-humble-moveit-setup-assistant

# Controllers
sudo apt install ros-humble-ros2-control ros-humble-ros2-controllers

# SLAM
sudo apt install ros-humble-slam-toolbox

# Simulation
sudo apt install ros-humble-gazebo-ros-pkgs ros-humble-gazebo-ros2-control

# Visualization
sudo apt install ros-humble-rviz2 ros-humble-joint-state-publisher-gui

# TF and URDF
sudo apt install ros-humble-xacro ros-humble-robot-state-publisher
```

### Python Dependencies
```bash
pip3 install pyserial opencv-python pyzbar numpy
```

---

## Workspace Structure

```
gripper_car_ws/
├── README.md                          # This file
├── src/
│   ├── applications/                  # High-level applications
│   │   └── pick_and_place/           # Pick-and-place state machine
│   │
│   ├── arm/                          # Robotic arm packages
│   │   ├── arm_description/          # Arm URDF and meshes
│   │   ├── arm_controller/           # Servo hardware bridge
│   │   └── arm_moveit/               # MoveIt2 configuration
│   │
│   ├── mobile_base/                  # Mobile base packages
│   │   ├── mobile_base_description/  # Base URDF and meshes
│   │   ├── base_controller/          # Motor hardware bridge
│   │   └── mobile_base_gazebo/       # Gazebo simulation
│   │
│   ├── mobile_manipulator/           # Combined system packages
│   │   ├── mobile_manipulator_description/  # Combined URDF
│   │   ├── mobile_manipulator_hardware/     # Unified hardware interface
│   │   ├── mobile_manipulator_navigation/   # Nav2 configuration
│   │   └── mobile_manipulator_application/  # Application nodes
│   │
│   ├── sensors/                      # Sensor packages
│   │   ├── camera_driver/            # Camera and QR detection
│   │   └── lidar_bringup/            # LiDAR launch wrapper
│   │
│   ├── bringup/                      # System coordination
│   │   └── robot_bringup/            # Master launch files
│   │
│   ├── simulation/                   # Simulation packages
│   │   ├── mobile_manipulator_sim/   # Full system simulation
│   │   └── navigation_sim/           # Navigation simulation
│   │
│   ├── ydlidar_ros2_driver/          # YDLiDAR ROS2 driver
│   └── ydlidar_ros2/                 # YDLiDAR SDK
│
└── arduino/                          # Arduino firmware
    └── mobile_manipulator_unified/   # Combined motor/servo control
```

---

## Quick Start

### 1. Clone and Build

```bash
# Create workspace (if not exists)
mkdir -p ~/gripper_car_ws/src
cd ~/gripper_car_ws

# Clone this repository
git clone <repository-url> src/

# Install dependencies
rosdep install --from-paths src --ignore-src -r -y

# Build
colcon build --symlink-install

# Source the workspace
source install/setup.bash
```

### 2. Run Simulation (No Hardware Required)

```bash
# Launch Gazebo simulation with full robot
ros2 launch mobile_manipulator_sim sim.launch.py

# In another terminal: Launch MoveIt for arm control
ros2 launch arm_moveit demo.launch.py
```

### 3. Run on Real Hardware

```bash
# On Raspberry Pi: Launch robot hardware
ros2 launch robot_bringup robot.launch.py serial_port:=/dev/ttyUSB0

# On development PC: Launch RViz for visualization
ros2 launch robot_bringup robot.launch.py use_rviz:=true
```

### 4. Run Complete Pick-and-Place Application

```bash
ros2 launch robot_bringup pick_and_place.launch.py
```

---

## Package Documentation

Each package contains its own README.md with detailed documentation. Below is a quick reference:

| Package | Description | Documentation |
|---------|-------------|---------------|
| **arm_description** | Arm URDF/meshes | [README](src/arm/arm_description/README.md) |
| **arm_controller** | Servo bridge | [README](src/arm/arm_controller/README.md) |
| **arm_moveit** | MoveIt config | [README](src/arm/arm_moveit/README.md) |
| **mobile_base_description** | Base URDF | [README](src/mobile_base/mobile_base_description/README.md) |
| **base_controller** | Motor bridge | [README](src/mobile_base/base_controller/README.md) |
| **mobile_manipulator_description** | Combined URDF | [README](src/mobile_manipulator/mobile_manipulator_description/README.md) |
| **mobile_manipulator_hardware** | Unified hardware | [README](src/mobile_manipulator/mobile_manipulator_hardware/README.md) |
| **mobile_manipulator_navigation** | Nav2 config | [README](src/mobile_manipulator/mobile_manipulator_navigation/README.md) |
| **robot_bringup** | Master launch | [README](src/bringup/robot_bringup/README.md) |
| **pick_and_place** | Application | [README](src/applications/pick_and_place/README.md) |
| **camera_driver** | Camera/QR | [README](src/sensors/camera_driver/README.md) |
| **mobile_manipulator_sim** | Simulation | [README](src/simulation/mobile_manipulator_sim/README.md) |

---

## Communication Patterns

### Topic Flow Diagram

```
                    ┌──────────────────────┐
                    │   robot_state_pub    │
                    │  (TF broadcaster)    │
                    └──────────┬───────────┘
                               │
                               │ subscribes
                               ▼
┌────────────────────────────────────────────────────────────────┐
│                      /joint_states                              │
│              (sensor_msgs/msg/JointState)                       │
└────────────────────────────────────────────────────────────────┘
                               ▲
                               │ publishes (aggregated)
                               │
                    ┌──────────┴───────────┐
                    │ joint_state_aggregator│
                    └──────────┬───────────┘
                               │ subscribes
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
     /base/joint_states  /arm/joint_states   (future sensors)
              ▲                ▲
              │                │
     base_hardware      arm_servo_bridge
         bridge


                    ┌──────────────────────┐
                    │      Nav2 Stack      │
                    │   (path planning)    │
                    └──────────┬───────────┘
                               │
                               │ publishes
                               ▼
                    ┌──────────────────────┐
                    │      /cmd_vel        │
                    │  (geometry_msgs/     │
                    │       Twist)         │
                    └──────────┬───────────┘
                               │
                               │ subscribes
                               ▼
                    ┌──────────────────────┐
                    │  base_hardware_      │
                    │  bridge              │──────────► Arduino
                    └──────────────────────┘            (motors)
```

### Key Topics

| Topic | Type | Publisher | Subscriber | Description |
|-------|------|-----------|------------|-------------|
| `/joint_states` | JointState | joint_state_aggregator | robot_state_publisher | All joint positions |
| `/cmd_vel` | Twist | Nav2/teleop | base_controller | Velocity commands |
| `/odom` | Odometry | base_controller | Nav2 | Wheel odometry |
| `/scan` | LaserScan | ydlidar_driver | Nav2/SLAM | LiDAR data |
| `/tf` | TFMessage | robot_state_pub | Everyone | Coordinate transforms |

---

## Troubleshooting

### Common Issues

**1. Serial port permission denied**
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

**2. LiDAR not detected**
```bash
# Check USB device
ls -la /dev/ttyUSB*

# Check permissions
sudo chmod 666 /dev/ttyUSB0
```

**3. Joint states not being published**
```bash
# Check if all controllers are running
ros2 topic list | grep joint

# Verify joint_state_aggregator is running
ros2 node list | grep aggregator
```

**4. MoveIt planning fails**
```bash
# Verify robot_state_publisher is running
ros2 topic echo /tf --once

# Check if URDF is loaded correctly
ros2 param get /robot_state_publisher robot_description
```

**5. Navigation not receiving odometry**
```bash
# Check odom topic
ros2 topic echo /odom --once

# Verify TF tree
ros2 run tf2_tools view_frames
```

### Diagnostic Commands

```bash
# View all active nodes
ros2 node list

# View all topics with types
ros2 topic list -t

# Check TF tree
ros2 run tf2_tools view_frames

# Monitor a specific topic
ros2 topic hz /joint_states
ros2 topic echo /joint_states

# Check node connections
ros2 node info /joint_state_aggregator
```

---

## Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch
3. Make your changes with clear commit messages
4. Submit a pull request

---

## License

This project is open source and available under the MIT License.

---

## Acknowledgments

- ROS2 Humble Hawksbill
- MoveIt2 Motion Planning Framework
- Navigation2 Stack
- YDLiDAR SDK and ROS2 Driver
- Gazebo Simulation
