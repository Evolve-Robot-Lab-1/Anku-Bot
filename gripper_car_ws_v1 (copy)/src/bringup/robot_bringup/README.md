# robot_bringup

Master bringup package providing high-level launch files for the complete mobile manipulator system.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Launch Files](#launch-files)
- [Usage](#usage)
- [System Modes](#system-modes)
- [Troubleshooting](#troubleshooting)

---

## Overview

This is the **main entry point** for running the mobile manipulator. It provides launch files that orchestrate all subsystems:

- Hardware interfaces (motors, servos, sensors)
- State publishers (URDF → TF)
- Navigation (SLAM, Nav2)
- Manipulation (MoveIt)
- Application nodes (pick and place)

### Launch File Hierarchy

```
robot_bringup/
├── pick_and_place.launch.py    ← Complete application
│   ├── robot.launch.py         ← Hardware + sensors
│   │   ├── robot_state_publisher
│   │   ├── hardware_node
│   │   └── ydlidar_driver
│   ├── nav2_bringup             ← Navigation
│   ├── arm_moveit/move_group    ← Manipulation
│   └── camera + app node        ← Vision + logic
│
├── navigation.launch.py         ← Navigation only
├── slam.launch.py               ← SLAM mapping
├── teleop.launch.py             ← Manual control
└── test_hardware.launch.py      ← Hardware testing
```

---

## Package Structure

```
robot_bringup/
├── CMakeLists.txt
├── package.xml
├── README.md
├── launch/
│   ├── robot.launch.py            # Hardware-only bringup
│   ├── pick_and_place.launch.py   # Complete application
│   ├── navigation.launch.py       # Nav2 only
│   ├── slam.launch.py             # SLAM mapping
│   ├── teleop.launch.py           # Keyboard control
│   └── test_hardware.launch.py    # Hardware testing
└── scripts/
    └── test_hardware_commands.py  # Hardware test utility
```

---

## Launch Files

### robot.launch.py

**Purpose**: Launch hardware interfaces only (for Raspberry Pi).

**Launches:**
- `robot_state_publisher` - URDF to TF
- `hardware_node` - Arduino bridge
- `ydlidar_driver` - LiDAR (optional)
- `rviz2` - Visualization (optional)

**Arguments:**

| Argument | Default | Description |
|----------|---------|-------------|
| `serial_port` | `/dev/ttyACM0` | Arduino port |
| `use_rviz` | `false` | Launch RViz |
| `use_lidar` | `true` | Launch LiDAR |

**Usage:**
```bash
# Basic hardware bringup
ros2 launch robot_bringup robot.launch.py

# With custom serial port
ros2 launch robot_bringup robot.launch.py serial_port:=/dev/ttyUSB0

# With RViz visualization
ros2 launch robot_bringup robot.launch.py use_rviz:=true

# Without LiDAR
ros2 launch robot_bringup robot.launch.py use_lidar:=false
```

---

### pick_and_place.launch.py

**Purpose**: Complete autonomous pick-and-place application.

**Launches:**
- `robot.launch.py` - Hardware
- `nav2_bringup` - Navigation stack
- `arm_moveit/move_group` - Motion planning
- `camera_driver` - Camera + QR detection
- `pick_and_place_node` - Application logic
- `rviz2` - Visualization

**Arguments:**

| Argument | Default | Description |
|----------|---------|-------------|
| `map` | (required) | Path to map YAML |
| `use_sim_time` | `false` | Simulation time |

**Usage:**
```bash
ros2 launch robot_bringup pick_and_place.launch.py map:=/path/to/map.yaml
```

---

### navigation.launch.py

**Purpose**: Navigation only (assumes hardware already running).

**Launches:**
- `nav2_bringup` - Full Nav2 stack
- AMCL localization

**Arguments:**

| Argument | Default | Description |
|----------|---------|-------------|
| `map` | (required) | Path to map YAML |
| `use_sim_time` | `false` | Simulation time |

**Usage:**
```bash
# First: launch hardware
ros2 launch robot_bringup robot.launch.py

# Then: launch navigation
ros2 launch robot_bringup navigation.launch.py map:=/path/to/map.yaml
```

---

### slam.launch.py

**Purpose**: SLAM mapping mode.

**Launches:**
- Hardware bringup
- SLAM Toolbox (async mapping)
- RViz for map visualization

**Usage:**
```bash
# Start SLAM mapping
ros2 launch robot_bringup slam.launch.py

# Drive robot around with teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Save map when done
ros2 run nav2_map_server map_saver_cli -f ~/my_map
```

---

### teleop.launch.py

**Purpose**: Manual keyboard control.

**Launches:**
- Keyboard teleop node

**Usage:**
```bash
# First: launch hardware
ros2 launch robot_bringup robot.launch.py

# Then: launch teleop
ros2 launch robot_bringup teleop.launch.py
```

---

### test_hardware.launch.py

**Purpose**: Hardware testing without full stack.

**Launches:**
- Robot state publisher
- Hardware node in test mode
- Optional: simulated encoder feedback

**Usage:**
```bash
ros2 launch robot_bringup test_hardware.launch.py
```

---

## Usage

### Quick Start: Basic Robot

```bash
# On Raspberry Pi
cd ~/gripper_car_ws
source install/setup.bash

# Launch hardware
ros2 launch robot_bringup robot.launch.py

# In another terminal: verify
ros2 topic list
ros2 topic echo /joint_states
```

### Quick Start: Navigation

```bash
# Terminal 1: Hardware
ros2 launch robot_bringup robot.launch.py

# Terminal 2: Navigation
ros2 launch robot_bringup navigation.launch.py map:=~/maps/my_map.yaml

# Terminal 3: Use RViz to send goals
rviz2
```

### Quick Start: Pick and Place

```bash
# Single command for complete application
ros2 launch robot_bringup pick_and_place.launch.py map:=~/maps/my_map.yaml
```

### Creating a Map

```bash
# Terminal 1: SLAM
ros2 launch robot_bringup slam.launch.py

# Terminal 2: Teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# After mapping: save
ros2 run nav2_map_server map_saver_cli -f ~/maps/new_map
```

---

## System Modes

### Mode 1: Development (PC + Pi)

```
┌─────────────────────────┐     ┌─────────────────────────┐
│     Raspberry Pi        │     │    Development PC       │
├─────────────────────────┤     ├─────────────────────────┤
│ robot.launch.py         │ ←── │ Navigation              │
│ - hardware_node         │ ROS │ - nav2_bringup          │
│ - robot_state_pub       │ DDS │ - rviz2                 │
│ - ydlidar_driver        │     │ - moveit                │
└─────────────────────────┘     └─────────────────────────┘
```

**Setup:**
```bash
# On Pi
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup robot.launch.py

# On PC (same network)
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup navigation.launch.py map:=...
```

### Mode 2: Standalone (Pi only)

```bash
# All nodes on Pi
ros2 launch robot_bringup pick_and_place.launch.py map:=...
```

### Mode 3: Simulation (PC only)

```bash
# Use simulation package instead
ros2 launch mobile_manipulator_sim sim.launch.py
```

---

## Test Scripts

### test_hardware_commands.py

Utility for testing hardware communication:

```bash
ros2 run robot_bringup test_hardware_commands.py
```

**Features:**
- Send test velocity commands
- Move arm to predefined poses
- Test gripper open/close
- Verify encoder feedback

---

## Troubleshooting

### Hardware Node Fails to Start

```bash
# Check serial port
ls -la /dev/ttyACM*

# Check permissions
sudo chmod 666 /dev/ttyACM0

# Check Arduino is connected
dmesg | tail -20
```

### LiDAR Not Publishing

```bash
# Check LiDAR topic
ros2 topic list | grep scan

# Verify driver is running
ros2 node list | grep ydlidar

# Check USB connection
ls -la /dev/ttyUSB*
```

### TF Tree Incomplete

```bash
# Generate TF tree
ros2 run tf2_tools view_frames

# Check for missing transforms
ros2 run tf2_ros tf2_monitor
```

### Navigation Not Working

1. Verify map is loaded:
```bash
ros2 topic echo /map --once
```

2. Check localization:
```bash
ros2 topic echo /amcl_pose
```

3. Verify all Nav2 nodes:
```bash
ros2 lifecycle list /bt_navigator
```

### MoveIt Planning Fails

```bash
# Check move_group is running
ros2 node list | grep move_group

# Verify controllers are available
ros2 action list | grep arm_controller
```

---

## Dependencies

- `mobile_manipulator_description` - Robot URDF
- `mobile_manipulator_hardware` - Hardware interface
- `mobile_manipulator_navigation` - Nav2 config
- `arm_moveit` - MoveIt config
- `camera_driver` - Camera driver
- `ydlidar_ros2_driver` - LiDAR driver
- `nav2_bringup` - Navigation stack
- `robot_state_publisher` - URDF to TF
- `rviz2` - Visualization

---

## Related Packages

- [mobile_manipulator_description](../mobile_manipulator/mobile_manipulator_description/README.md) - Robot model
- [mobile_manipulator_hardware](../mobile_manipulator/mobile_manipulator_hardware/README.md) - Hardware interface
- [mobile_manipulator_navigation](../mobile_manipulator/mobile_manipulator_navigation/README.md) - Navigation
- [pick_and_place](../applications/pick_and_place/README.md) - Application logic
