# mobile_manipulator_hardware

ROS2 package providing a unified hardware interface for the complete mobile manipulator (arm + base) via a single Arduino connection.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Hardware Node](#hardware-node)
- [Communication Protocol](#communication-protocol)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package provides a single ROS2 node that interfaces with the unified Arduino firmware controlling both:

- **Robotic Arm**: 6 servo motors via PCA9685
- **Mobile Base**: 4 DC motors with encoders

### Advantages of Unified Interface

| Separate Controllers | Unified Controller |
|---------------------|-------------------|
| Two serial ports needed | Single serial port |
| Separate clock domains | Synchronized timing |
| Complex coordination | Atomic commands |
| Higher latency | Lower latency |

### System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                          ROS2                                        │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │
│  │   MoveIt    │    │    Nav2     │    │   Teleop    │              │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘              │
│         │                   │                  │                     │
│         ▼                   ▼                  ▼                     │
│  /arm/joint_commands      /cmd_vel                                   │
│         │                   │                                        │
│         └───────────────────┼───────────────────────────────────────│
│                             ▼                                        │
│              ┌────────────────────────────┐                          │
│              │    hardware_node.py        │                          │
│              │   (MobileManipulatorHW)    │                          │
│              └─────────────┬──────────────┘                          │
│                            │ Serial                                  │
└────────────────────────────┼─────────────────────────────────────────┘
                             ▼
                    ┌─────────────────┐
                    │  Arduino Mega   │
                    │  Unified Firmware│
                    └────────┬────────┘
                             │
               ┌─────────────┴─────────────┐
               ▼                           ▼
        ┌───────────┐               ┌───────────┐
        │  PCA9685  │               │  L298N    │
        │  (Servos) │               │  (Motors) │
        └───────────┘               └───────────┘
```

---

## Package Structure

```
mobile_manipulator_hardware/
├── package.xml
├── setup.py
├── setup.cfg
├── README.md
├── resource/
│   └── mobile_manipulator_hardware
├── mobile_manipulator_hardware/
│   ├── __init__.py
│   └── hardware_node.py            # Main unified hardware node
├── launch/
│   └── hardware.launch.py          # Launch file
└── config/
    └── hardware_params.yaml        # Complete configuration
```

---

## Hardware Node

### Purpose

Single interface to Arduino handling:
- Arm servo position commands
- Base velocity commands
- Encoder odometry calculation
- Joint state publishing
- TF broadcasting (odom → body_link)

### Topics

**Subscribed:**

| Topic | Type | Description |
|-------|------|-------------|
| `/arm/joint_commands` | `JointState` | Arm joint positions (radians) |
| `/cmd_vel` | `Twist` | Base velocity (m/s, rad/s) |
| `/hardware/command` | `String` | Raw debug commands |

**Published:**

| Topic | Type | Description |
|-------|------|-------------|
| `/joint_states` | `JointState` | All joints (arm + wheels + mimic) |
| `/odom` | `Odometry` | Wheel odometry |
| `/hardware/status` | `String` | Status messages |

**TF Broadcasts:**

| Parent | Child | Description |
|--------|-------|-------------|
| `odom` | `body_link` | Robot pose from encoders |

### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `serial_port` | string | `/dev/ttyACM0` | Arduino port |
| `baud_rate` | int | `115200` | Serial baud rate |
| `arm_joints` | list | `[joint_1,...]` | Arm joint names |
| `wheel_joints` | list | `[front_left_joint,...]` | Wheel joint names |
| `wheel_radius` | float | `0.075` | Wheel radius (m) |
| `wheel_base` | float | `0.35` | Wheel separation (m) |
| `encoder_cpr` | int | `360` | Encoder counts per revolution |
| `base_frame` | string | `body_link` | Base TF frame |
| `odom_frame` | string | `odom` | Odom TF frame |
| `publish_odom_tf` | bool | `true` | Publish odom→base TF |
| `joint_state_rate` | float | `50.0` | Publishing rate (Hz) |

---

## Communication Protocol

### ROS2 → Arduino Commands

| Command | Format | Description |
|---------|--------|-------------|
| Arm position | `ARM,a1,a2,a3,a4,a5,a6` | Set servo angles (0-180°) |
| Base velocity | `VEL,vx,wz` | Set velocity (m/s, rad/s) |
| Stop all | `STOP` | Emergency stop |
| Status | `STATUS` | Request status |

### Arduino → ROS2 Messages

| Message | Format | Description |
|---------|--------|-------------|
| Arm position | `ARM_POS,a1,a2,a3,a4,a5,a6` | Current servo angles |
| Encoders | `ENC,fl,fr,bl,br` | Encoder tick counts |
| Status | `STATUS,<msg>` | Status message |
| Heartbeat | `HEARTBEAT,<ms>` | Arduino uptime |
| OK | `OK,<cmd>` | Command acknowledgment |
| Error | `ERROR,<msg>` | Error report |

---

## Usage

### Launch Hardware Node

```bash
ros2 launch mobile_manipulator_hardware hardware.launch.py

# With custom port
ros2 launch mobile_manipulator_hardware hardware.launch.py serial_port:=/dev/ttyUSB0
```

### Test Arm Control

```bash
# Move joint_1 to 0.5 radians
ros2 topic pub /arm/joint_commands sensor_msgs/JointState \
  "{name: ['joint_1'], position: [0.5]}"

# Open gripper
ros2 topic pub /arm/joint_commands sensor_msgs/JointState \
  "{name: ['left_gear_joint'], position: [0.8]}"
```

### Test Base Control

```bash
# Move forward
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.2}}"

# Turn left
ros2 topic pub /cmd_vel geometry_msgs/Twist "{angular: {z: 0.5}}"

# Stop
ros2 topic pub /cmd_vel geometry_msgs/Twist "{}"
```

### Monitor Status

```bash
# View joint states
ros2 topic echo /joint_states

# View odometry
ros2 topic echo /odom

# Check hardware status
ros2 topic echo /hardware/status
```

### Send Raw Commands

```bash
# Direct Arduino command (debugging)
ros2 topic pub /hardware/command std_msgs/String "data: 'STATUS'"
```

---

## Configuration

### hardware_params.yaml

```yaml
hardware_node:
  ros__parameters:
    # Serial connection
    serial_port: "/dev/ttyACM0"
    baud_rate: 115200

    # Arm configuration
    arm_joints:
      - joint_1
      - joint_2
      - joint_3
      - joint_4
      - gripper_base_joint
      - left_gear_joint

    # Wheel configuration
    wheel_joints:
      - front_left_joint
      - front_right_joint
      - back_left_joint
      - back_right_joint

    # Physical dimensions
    wheel_radius: 0.075
    wheel_base: 0.35
    encoder_cpr: 360

    # TF frames
    base_frame: "body_link"
    odom_frame: "odom"
    publish_odom_tf: true

    # Publishing rate
    joint_state_rate: 50.0

    # Odometry covariance
    pose_covariance_diagonal: [0.01, 0.01, 0.01, 0.01, 0.01, 0.03]
    twist_covariance_diagonal: [0.01, 0.01, 0.01, 0.01, 0.01, 0.03]
```

---

## Understanding the Code

### Arm Position Conversion

Converts ROS2 joint angles (radians) to servo angles (0-180°):

```python
def joint_to_servo_angle(self, joint_idx, position_rad):
    # Get joint limits from URDF
    lower, upper = self.arm_joint_limits[joint_name]

    # Clamp to limits
    position_rad = max(lower, min(upper, position_rad))

    # Gripper: special mapping
    if joint_idx == 5:  # left_gear_joint
        angle = 90.0 + (position_rad * 180.0 / pi)
    else:
        # General: center of joint range → 90°
        joint_range = upper - lower
        center = (upper + lower) / 2.0
        angle = 90.0 + ((position_rad - center) / (joint_range / 2.0)) * 90.0

    return max(0.0, min(180.0, angle))
```

### Odometry Calculation

Differential drive kinematics from encoder counts:

```python
def update_odometry(self, counts):
    # Average left/right encoder deltas
    left_delta = (counts[0] + counts[2]) / 2.0 - (last[0] + last[2]) / 2.0
    right_delta = (counts[1] + counts[3]) / 2.0 - (last[1] + last[3]) / 2.0

    # Convert to distance
    left_dist = left_delta * meters_per_tick
    right_dist = right_delta * meters_per_tick

    # Compute motion
    distance = (left_dist + right_dist) / 2.0
    delta_theta = (right_dist - left_dist) / wheel_base

    # Update pose
    x += distance * cos(theta)
    y += distance * sin(theta)
    theta += delta_theta
```

### Mimic Joint Handling

Gripper mimic joints are calculated and published:

```python
mimic_joints = {
    'right_gear_joint': ('left_gear_joint', -1.0),
    'left_finger_joint': ('left_gear_joint', 1.0),
    ...
}

# In publish_joint_states:
for mimic_name, (parent_name, multiplier) in self.mimic_joints.items():
    parent_idx = self.arm_joints.index(parent_name)
    mimic_pos = self.arm_positions[parent_idx] * multiplier
    all_names.append(mimic_name)
    all_positions.append(mimic_pos)
```

---

## Troubleshooting

### Serial Connection Failed

```bash
# Check device exists
ls -la /dev/ttyACM*

# Check permissions
sudo chmod 666 /dev/ttyACM0

# Permanent fix: add to dialout group
sudo usermod -a -G dialout $USER
```

### No Arm Response

1. Verify Arduino firmware is unified version
2. Check serial communication:
```bash
ros2 topic echo /hardware/status
```
3. Send test command:
```bash
ros2 topic pub /hardware/command std_msgs/String "data: 'STATUS'"
```

### Odometry Drift

1. Calibrate wheel_radius and wheel_base
2. Verify encoder_cpr matches your encoders
3. Check for wheel slippage

### Wrong Joint Names

Ensure names in parameters match URDF:
```bash
ros2 param get /hardware_node arm_joints
ros2 param get /hardware_node wheel_joints
```

---

## Dependencies

- `rclpy` - ROS2 Python client
- `sensor_msgs` - JointState messages
- `geometry_msgs` - Twist, TransformStamped
- `nav_msgs` - Odometry
- `std_msgs` - String
- `tf2_ros` - TF broadcaster
- `pyserial` - Serial communication

---

## Related Packages

- [mobile_manipulator_description](../mobile_manipulator_description/README.md) - Combined URDF
- [arm_controller](../../arm/arm_controller/README.md) - Separate arm control
- [base_controller](../../mobile_base/base_controller/README.md) - Separate base control
- [robot_bringup](../../bringup/robot_bringup/README.md) - System integration
