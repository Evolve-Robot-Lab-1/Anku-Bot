# mobile_manipulator_description

ROS2 package that combines mobile base and robotic arm into a unified robot model with joint state aggregation.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Joint State Aggregator](#joint-state-aggregator)
- [URDF Model](#urdf-model)
- [Launch Files](#launch-files)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package integrates the mobile base and robotic arm subsystems into a complete mobile manipulator. It provides:

- **Combined URDF** model including base, arm, and sensors
- **Joint State Aggregator** to merge separate joint state topics
- **Robot Bringup** launch file for the complete hardware system

### Why Joint State Aggregation?

The robot has multiple hardware controllers publishing to separate topics:
- Base controller → `/base/joint_states` (wheel joints)
- Arm controller → `/arm/joint_states` (arm + gripper joints)

The `robot_state_publisher` requires a single `/joint_states` topic. The aggregator merges these into one.

```
/base/joint_states ───┐
                      ├──► Joint State Aggregator ──► /joint_states ──► robot_state_publisher
/arm/joint_states ────┘
```

---

## Package Structure

```
mobile_manipulator_description/
├── CMakeLists.txt
├── package.xml
├── README.md
├── urdf/
│   └── mobile_manipulator.urdf.xacro    # Combined robot model
├── scripts/
│   └── joint_state_aggregator.py        # Merges joint state topics
├── launch/
│   ├── robot_bringup.launch.py          # Complete hardware bringup
│   ├── display.launch.py                # RViz visualization
│   └── arm_manipulation.launch.py       # Arm manipulation demo
└── config/
    ├── ros2_controllers.yaml            # Controller configuration
    └── display.rviz                     # RViz config
```

---

## Joint State Aggregator

### Purpose

Temporary solution to combine joint states from multiple hardware controllers until migration to ros2_control.

### Topics

**Subscribed:**

| Topic | Type | Description |
|-------|------|-------------|
| `/base/joint_states` | `JointState` | Wheel joints from base_controller |
| `/arm/joint_states` | `JointState` | Arm/gripper joints from arm_controller |

**Published:**

| Topic | Type | Description |
|-------|------|-------------|
| `/joint_states` | `JointState` | Combined all robot joints |

### Behavior

1. Subscribes to both source topics (best-effort QoS)
2. Stores latest message from each source
3. At 50 Hz, merges and publishes combined message
4. Waits until both sources have data before publishing

### Code Overview

```python
def publish_combined(self):
    """Combine and publish unified joint states"""

    # Wait until we have data from both subsystems
    if self.base_joint_state is None or self.arm_joint_state is None:
        return

    # Create combined message
    combined = JointState()
    combined.header.stamp = self.get_clock().now().to_msg()

    # Merge base joints
    combined.name.extend(self.base_joint_state.name)
    combined.position.extend(self.base_joint_state.position)

    # Merge arm joints
    combined.name.extend(self.arm_joint_state.name)
    combined.position.extend(self.arm_joint_state.position)

    self.joint_state_pub.publish(combined)
```

### Migration Path

For production, migrate to ros2_control with `joint_state_broadcaster`:

```yaml
# Future: ros2_controllers.yaml
joint_state_broadcaster:
  type: joint_state_broadcaster/JointStateBroadcaster
```

---

## URDF Model

The combined model includes:

- Mobile base from `mobile_base_description`
- Robotic arm from `arm_description`
- LiDAR mount and sensor
- Camera mount

### Joint List

**Wheel Joints (from base):**
- `front_left_joint`
- `front_right_joint`
- `back_left_joint`
- `back_right_joint`

**Arm Joints:**
- `joint_1` (base rotation)
- `joint_2` (shoulder)
- `joint_3` (elbow)
- `joint_4` (wrist)
- `gripper_base_joint` (wrist rotation)
- `left_gear_joint` (gripper)

**Mimic Joints (gripper mechanism):**
- `right_gear_joint`
- `left_finger_joint`
- `right_finger_joint`
- `left_joint`
- `right_joint`

---

## Launch Files

### robot_bringup.launch.py

**Purpose**: Complete hardware bringup on Raspberry Pi.

**Launches:**
1. Joint State Aggregator (creates `/joint_states`)
2. Robot State Publisher (reads URDF, publishes TF)
3. Base Controller (motor control, odometry)
4. Arm Controller (servo control)
5. LiDAR Driver (optional, commented out)

**Usage:**
```bash
ros2 launch mobile_manipulator_description robot_bringup.launch.py
```

**Arguments:**

| Argument | Default | Description |
|----------|---------|-------------|
| `use_sim_time` | `false` | Use simulation time |
| `serial_port` | `/dev/ttyACM0` | Arduino serial port |

### display.launch.py

**Purpose**: Visualize robot model in RViz.

**Launches:**
- Robot State Publisher
- Joint State Publisher GUI
- RViz2

```bash
ros2 launch mobile_manipulator_description display.launch.py
```

---

## Usage

### Hardware Bringup (On Raspberry Pi)

```bash
# Source workspace
source ~/gripper_car_ws/install/setup.bash

# Launch complete robot
ros2 launch mobile_manipulator_description robot_bringup.launch.py

# With custom serial port
ros2 launch mobile_manipulator_description robot_bringup.launch.py serial_port:=/dev/ttyUSB0
```

### Verify Joint States

```bash
# Check combined joint states
ros2 topic echo /joint_states

# Verify all joints are present
ros2 topic echo /joint_states --field name

# Check publishing rate
ros2 topic hz /joint_states
```

### View TF Tree

```bash
# Generate TF tree diagram
ros2 run tf2_tools view_frames

# Check specific transform
ros2 run tf2_ros tf2_echo odom body_link
```

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         robot_bringup.launch.py                          │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        ▼                           ▼                           ▼
┌───────────────┐          ┌───────────────┐          ┌───────────────┐
│ base_control  │          │ arm_control   │          │   LiDAR       │
│ .launch.py    │          │ .launch.py    │          │ (optional)    │
└───────┬───────┘          └───────┬───────┘          └───────────────┘
        │                          │
        ▼                          ▼
  /base/joint_states        /arm/joint_states
        │                          │
        └───────────┬──────────────┘
                    ▼
         ┌────────────────────┐
         │ joint_state_       │
         │ aggregator.py      │
         └──────────┬─────────┘
                    ▼
              /joint_states
                    │
                    ▼
         ┌────────────────────┐
         │ robot_state_       │
         │ publisher          │
         └──────────┬─────────┘
                    ▼
                  /tf
```

---

## Troubleshooting

### No Joint States Being Published

1. Check aggregator is running:
```bash
ros2 node list | grep aggregator
```

2. Verify source topics:
```bash
ros2 topic echo /base/joint_states --once
ros2 topic echo /arm/joint_states --once
```

3. Check for errors in aggregator:
```bash
# Look for warning about waiting for data
ros2 run mobile_manipulator_description joint_state_aggregator.py
```

### TF Tree Incomplete

1. Verify robot_state_publisher has URDF:
```bash
ros2 param get /robot_state_publisher robot_description
```

2. Check joint names match URDF:
```bash
ros2 topic echo /joint_states --field name
```

### Timing Issues

If joint states are stale or TF has warnings:

1. Check clock synchronization
2. Verify `use_sim_time` is consistent across all nodes
3. Check aggregator publish rate (50 Hz default)

---

## Dependencies

- `mobile_base_description` - Base URDF
- `arm_description` - Arm URDF
- `base_controller` - Motor control
- `arm_controller` - Servo control
- `robot_state_publisher` - TF from URDF
- `sensor_msgs` - JointState messages
- `ydlidar_ros2_driver` - LiDAR (optional)

---

## Related Packages

- [mobile_base_description](../../mobile_base/mobile_base_description/README.md) - Base URDF
- [arm_description](../../arm/arm_description/README.md) - Arm URDF
- [mobile_manipulator_hardware](../mobile_manipulator_hardware/README.md) - Unified hardware interface
- [robot_bringup](../../bringup/robot_bringup/README.md) - Complete system bringup
