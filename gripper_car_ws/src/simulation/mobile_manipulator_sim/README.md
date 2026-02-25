# mobile_manipulator_sim

ROS2 package for complete Gazebo simulation of the mobile manipulator with SLAM, navigation, and MoveIt2 integration.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Launch Options](#launch-options)
- [Simulation Modes](#simulation-modes)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package provides a full-featured simulation environment for developing and testing the mobile manipulator without hardware. It includes:

- **Gazebo physics simulation** with differential drive
- **SLAM Toolbox** for mapping
- **Nav2** for autonomous navigation
- **MoveIt2** for arm motion planning
- **Simulated sensors** (LiDAR, camera)

### Why Simulate?

- Develop algorithms without hardware risk
- Test navigation in various environments
- Debug motion planning edge cases
- Train machine learning models
- Validate changes before deployment

---

## Package Structure

```
mobile_manipulator_sim/
├── CMakeLists.txt
├── package.xml
├── README.md
├── urdf/
│   ├── mobile_manipulator.urdf.xacro    # Simulation URDF
│   └── gazebo.xacro                     # Gazebo plugins
├── launch/
│   ├── sim.launch.py                    # Main simulation launcher
│   └── moveit_gazebo.launch.py          # MoveIt for simulation
├── config/
│   ├── sim.rviz                         # RViz configuration
│   ├── sim_ros2_controllers.yaml        # Simulation controllers
│   └── sim_moveit_controllers.yaml      # MoveIt controller config
└── worlds/
    └── (inherited from mobile_base_gazebo)
```

---

## Launch Options

### Main Launch File: sim.launch.py

| Argument | Default | Description |
|----------|---------|-------------|
| `slam` | `true` | Enable SLAM mapping (false = use map) |
| `use_nav` | `true` | Enable Nav2 stack |
| `use_moveit` | `false` | Enable MoveIt2 arm control |
| `use_rviz` | `true` | Launch RViz visualization |
| `use_teleop` | `true` | Launch keyboard teleop |
| `world` | `test_arena` | Gazebo world name |
| `map` | `""` | Map file for localization mode |
| `x` | `-3.0` | Initial X position |
| `y` | `-3.0` | Initial Y position |
| `z` | `0.1` | Initial Z position |
| `yaw` | `0.0` | Initial heading (radians) |

---

## Simulation Modes

### Mode 1: SLAM Mapping

Create maps of the simulated environment:

```bash
ros2 launch mobile_manipulator_sim sim.launch.py slam:=true

# Drive robot with keyboard teleop (opens automatically in xterm)
# Save map when done:
ros2 run nav2_map_server map_saver_cli -f ~/sim_map
```

### Mode 2: Navigation with Map

Navigate using a pre-built map:

```bash
ros2 launch mobile_manipulator_sim sim.launch.py \
  slam:=false \
  map:=/path/to/map.yaml
```

### Mode 3: Arm Control Only

Test MoveIt without navigation:

```bash
ros2 launch mobile_manipulator_sim sim.launch.py \
  use_nav:=false \
  use_moveit:=true
```

### Mode 4: Full Stack

Complete system with navigation and manipulation:

```bash
ros2 launch mobile_manipulator_sim sim.launch.py \
  use_nav:=true \
  use_moveit:=true \
  slam:=false \
  map:=/path/to/map.yaml
```

---

## Usage

### Quick Start: Basic Simulation

```bash
# Launch simulation with SLAM and teleop
ros2 launch mobile_manipulator_sim sim.launch.py

# Wait for Gazebo to load (~10 seconds)
# Use keyboard in teleop window (WASD keys)
# Watch RViz for SLAM map building
```

### Test Navigation

```bash
# Create a map first (see Mode 1)
# Then launch with map:
ros2 launch mobile_manipulator_sim sim.launch.py \
  slam:=false \
  map:=~/sim_map.yaml

# In RViz:
# 1. Set initial pose with "2D Pose Estimate"
# 2. Send goals with "2D Goal Pose"
```

### Test Arm with MoveIt

```bash
ros2 launch mobile_manipulator_sim sim.launch.py use_moveit:=true

# In RViz:
# 1. Select "arm" planning group
# 2. Drag interactive markers
# 3. Click "Plan & Execute"
```

### Spawn in Different World

```bash
# Available worlds: empty, test_arena, qr_test_arena
ros2 launch mobile_manipulator_sim sim.launch.py world:=qr_test_arena
```

### Custom Spawn Position

```bash
ros2 launch mobile_manipulator_sim sim.launch.py \
  x:=0.0 y:=0.0 yaw:=1.57
```

---

## Startup Sequence

The launch file uses timed actions to ensure proper startup order:

```
Time 0s:  robot_state_publisher starts
          (publishes URDF for Gazebo to read)

Time 2s:  Gazebo launches
          (loads world and waits for spawn)

Time 4s:  RViz launches
          (visualization ready)

Time 5s:  Teleop launches (in xterm)
          (keyboard control available)

Time 7s:  Robot spawns in Gazebo
          (entity created from robot_description)

Time 8s:  SLAM or Map Server starts
          (mapping or localization)

Time 10s: Nav2 nodes start
          (controller, planner, behaviors)

Time 12s: MoveIt starts (if enabled)
          (arm motion planning)

Time 14s: Lifecycle manager starts Nav2 nodes
          (brings nodes to active state)
```

---

## Configuration

### Simulated Sensors

**LiDAR** (from mobile_base_gazebo):
- 360° scan
- 10 Hz update rate
- 0.12m - 12m range
- Topic: `/scan`

**Camera** (from mobile_base_gazebo):
- 640x480 @ 30 FPS
- Topic: `/camera/image_raw`

### Controller Configuration

The simulation uses `gazebo_ros2_control` instead of hardware drivers:

```yaml
# sim_ros2_controllers.yaml
controller_manager:
  ros__parameters:
    update_rate: 100

    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster

    arm_controller:
      type: joint_trajectory_controller/JointTrajectoryController

    gripper_controller:
      type: position_controllers/GripperActionController
```

### MoveIt for Simulation

The `moveit_gazebo.launch.py` configures MoveIt to use simulation controllers:
- No standalone `ros2_control_node` (Gazebo provides hardware abstraction)
- Uses `gazebo_ros2_control` plugin in URDF
- Action servers provided by simulation controllers

---

## Understanding the Code

### sim.launch.py Key Sections

#### Timed Actions

```python
# Delay actions to ensure dependencies are ready
TimerAction(
    period=7.0,  # Wait 7 seconds
    actions=[
        Node(
            package='gazebo_ros',
            executable='spawn_entity.py',
            arguments=['-entity', 'mobile_manipulator', '-topic', 'robot_description']
        )
    ]
)
```

#### Conditional Launch

```python
# SLAM only in mapping mode
Node(
    package='slam_toolbox',
    executable='async_slam_toolbox_node',
    condition=IfCondition(
        PythonExpression(["'", slam, "' == 'true' and '", use_nav, "' == 'true'"])
    )
)

# Map server only in localization mode
Node(
    package='nav2_map_server',
    executable='map_server',
    condition=IfCondition(
        PythonExpression(["'", slam, "' == 'false' and '", use_nav, "' == 'true'"])
    )
)
```

### Gazebo Plugin Configuration

In `urdf/gazebo.xacro`:

```xml
<gazebo>
  <plugin name="gazebo_ros2_control" filename="libgazebo_ros2_control.so">
    <parameters>$(find mobile_manipulator_sim)/config/sim_ros2_controllers.yaml</parameters>
  </plugin>
</gazebo>
```

---

## Troubleshooting

### Gazebo Not Starting

```bash
# Check Gazebo installation
gazebo --version

# Check for GPU issues (try software rendering)
export LIBGL_ALWAYS_SOFTWARE=1
ros2 launch mobile_manipulator_sim sim.launch.py
```

### Robot Not Spawning

1. Verify robot_state_publisher is running:
```bash
ros2 topic echo /robot_description --once
```

2. Check Gazebo spawn output for errors

3. Verify URDF is valid:
```bash
check_urdf <(xacro path/to/mobile_manipulator.urdf.xacro sim_mode:=true)
```

### Navigation Not Working

1. Check if all Nav2 nodes are active:
```bash
ros2 lifecycle list /controller_server
ros2 lifecycle list /planner_server
```

2. Verify SLAM or map is publishing:
```bash
ros2 topic echo /map --once
```

3. Check TF tree:
```bash
ros2 run tf2_tools view_frames
```

### MoveIt Planning Fails

1. Verify controllers are loaded:
```bash
ros2 control list_controllers
```

2. Check joint states:
```bash
ros2 topic echo /joint_states
```

3. Ensure arm_controller is active

### Simulation Too Slow

1. Use simpler world (`world:=empty`)
2. Reduce sensor update rates
3. Disable visualization:
```bash
ros2 launch mobile_manipulator_sim sim.launch.py use_rviz:=false
```

---

## Dependencies

- `gazebo_ros` - Gazebo-ROS2 integration
- `gazebo_ros2_control` - ros2_control for Gazebo
- `mobile_base_gazebo` - Base simulation (worlds, plugins)
- `mobile_manipulator_navigation` - Nav2 configuration
- `arm_moveit` - MoveIt configuration
- `slam_toolbox` - SLAM mapping
- `nav2_bringup` - Navigation stack

---

## Related Packages

- [mobile_base_gazebo](../../mobile_base/mobile_base_gazebo/README.md) - Base simulation
- [mobile_manipulator_navigation](../../mobile_manipulator/mobile_manipulator_navigation/README.md) - Navigation config
- [arm_moveit](../../arm/arm_moveit/README.md) - MoveIt configuration
- [robot_bringup](../../bringup/robot_bringup/README.md) - Hardware bringup
