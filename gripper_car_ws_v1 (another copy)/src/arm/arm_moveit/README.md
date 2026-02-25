# arm_moveit

MoveIt2 configuration package for the 6-DOF robotic arm. Provides motion planning, collision detection, and trajectory execution capabilities.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [MoveIt Concepts](#moveit-concepts)
- [Planning Groups](#planning-groups)
- [Configuration Files](#configuration-files)
- [Launch Files](#launch-files)
- [Usage](#usage)
- [Integration with Hardware](#integration-with-hardware)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package was generated using the MoveIt Setup Assistant and customized for integration with servo-based hardware. It provides:

- **Motion Planning**: Automatic path planning avoiding obstacles
- **Inverse Kinematics**: Calculate joint positions for desired end-effector poses
- **Collision Detection**: Prevent self-collision and environment collision
- **Trajectory Execution**: Interface with hardware controllers

### What is MoveIt2?

MoveIt2 is a motion planning framework that answers the question: "How should the robot move its joints to reach a goal pose without hitting anything?"

```
Goal Pose (x, y, z, orientation)
           │
           ▼
    ┌──────────────┐
    │   MoveIt2    │
    │   Planner    │
    └──────┬───────┘
           │
           ▼
Joint Trajectory [q1(t), q2(t), ..., q6(t)]
           │
           ▼
    ┌──────────────┐
    │  Controller  │
    │   Manager    │
    └──────┬───────┘
           │
           ▼
Hardware Execution (trajectory_bridge → servo_bridge → Arduino)
```

---

## Package Structure

```
arm_moveit/
├── CMakeLists.txt
├── package.xml
├── README.md
├── config/
│   ├── mobile_arm_bot_2.urdf.xacro    # Robot URDF (reference)
│   ├── mobile_arm_bot_2.srdf          # Semantic robot description
│   ├── kinematics.yaml                # IK solver configuration
│   ├── joint_limits.yaml              # Joint limit overrides
│   ├── pilz_cartesian_limits.yaml     # Cartesian motion limits
│   ├── initial_positions.yaml         # Default joint positions
│   ├── moveit_controllers.yaml        # Controller configuration
│   ├── ros2_controllers.yaml          # ros2_control configuration
│   └── sensors_3d.yaml                # 3D sensor integration
├── launch/
│   ├── demo.launch.py                 # Interactive demo (no hardware)
│   ├── move_group.launch.py           # MoveIt move_group node
│   ├── moveit_rviz.launch.py          # RViz with MoveIt plugin
│   ├── real_hardware.launch.py        # Real hardware integration
│   ├── rsp.launch.py                  # Robot State Publisher
│   ├── spawn_controllers.launch.py    # Controller spawner
│   ├── setup_assistant.launch.py      # MoveIt Setup Assistant
│   ├── warehouse_db.launch.py         # Motion database
│   └── static_virtual_joint_tfs.launch.py
└── .setup_assistant                   # Setup Assistant metadata
```

---

## MoveIt Concepts

### SRDF (Semantic Robot Description Format)

The SRDF file (`mobile_arm_bot_2.srdf`) extends the URDF with semantic information:

1. **Planning Groups**: Named collections of joints for coordinated motion
2. **End Effectors**: Definition of gripper/tool
3. **Virtual Joints**: Connection to world frame
4. **Disabled Collisions**: Pairs of links that can't collide (optimization)

### Move Group

The central MoveIt node that provides:
- Motion planning services
- Trajectory execution
- Robot state monitoring
- Scene management

### Controller Manager

Interfaces between MoveIt trajectories and hardware:
```
MoveIt Trajectory → Controller Manager → FollowJointTrajectory Action → trajectory_bridge
```

---

## Planning Groups

### arm (5 joints)

Main arm group for positioning the gripper in 3D space.

| Joint | Description |
|-------|-------------|
| `joint_1` | Base rotation |
| `joint_2` | Shoulder pitch |
| `joint_3` | Elbow pitch |
| `joint_4` | Wrist pitch |
| `gripper_base_joint` | Wrist roll |

**End Effector Link**: `gripper_base_link`

### gripper (1 joint)

Gripper control group for open/close operations.

| Joint | Description |
|-------|-------------|
| `left_gear_joint` | Gripper actuator (mimic joints follow) |

---

## Configuration Files

### kinematics.yaml

Configures the inverse kinematics solver:

```yaml
arm:
  kinematics_solver: kdl_kinematics_plugin/KDLKinematicsPlugin
  kinematics_solver_search_resolution: 0.005
  kinematics_solver_timeout: 0.5
```

**KDL (Kinematics and Dynamics Library)**: Default solver, works well for most configurations.

**Parameters Explained**:
- `search_resolution`: Granularity for joint space search (smaller = more precise, slower)
- `timeout`: Maximum time to find a solution

### joint_limits.yaml

Override URDF joint limits for planning safety:

```yaml
joint_limits:
  joint_1:
    has_velocity_limits: true
    max_velocity: 2.0
    has_acceleration_limits: true
    max_acceleration: 5.0
```

### moveit_controllers.yaml

Defines how MoveIt sends commands to controllers:

```yaml
moveit_controller_manager: moveit_simple_controller_manager/MoveItSimpleControllerManager

moveit_simple_controller_manager:
  controller_names:
    - arm_controller
    - gripper_controller

  arm_controller:
    type: FollowJointTrajectory
    joints:
      - joint_1
      - joint_2
      - joint_3
      - joint_4
      - gripper_base_joint
    action_ns: follow_joint_trajectory
    default: true

  gripper_controller:
    type: GripperCommand
    joints:
      - left_gear_joint
    action_ns: gripper_cmd
    default: true
```

**Controller Types**:
- `FollowJointTrajectory`: For smooth multi-point trajectories
- `GripperCommand`: For simple open/close commands

---

## Launch Files

### demo.launch.py

**Purpose**: Interactive demonstration without real hardware.

**What it launches**:
1. Robot State Publisher (publishes TF from URDF)
2. Move Group (motion planning)
3. RViz with MoveIt plugin
4. Joint State Publisher GUI (manual joint control)

```bash
ros2 launch arm_moveit demo.launch.py
```

### move_group.launch.py

**Purpose**: Launch just the move_group planning node.

**Use case**: When you want MoveIt planning without RViz (e.g., autonomous operation).

```bash
ros2 launch arm_moveit move_group.launch.py
```

### real_hardware.launch.py

**Purpose**: MoveIt with actual robot hardware.

**What it launches**:
1. Move Group
2. Robot State Publisher
3. Controller spawners
4. Hardware interface nodes

```bash
ros2 launch arm_moveit real_hardware.launch.py
```

### moveit_rviz.launch.py

**Purpose**: Launch RViz with MoveIt plugin only.

**Use case**: Connect to an already-running move_group.

```bash
ros2 launch arm_moveit moveit_rviz.launch.py
```

### setup_assistant.launch.py

**Purpose**: Re-run MoveIt Setup Assistant to modify configuration.

```bash
ros2 launch arm_moveit setup_assistant.launch.py
```

---

## Usage

### Interactive Demo (No Hardware)

```bash
# Launch the demo
ros2 launch arm_moveit demo.launch.py
```

In RViz:
1. Use the interactive markers (colored rings) to drag the end-effector
2. Click "Plan" to compute a trajectory
3. Click "Execute" to simulate the motion

### Plan and Execute from Python

```python
#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from moveit_py import MoveIt
from geometry_msgs.msg import PoseStamped

class ArmPlanner(Node):
    def __init__(self):
        super().__init__('arm_planner')

        # Initialize MoveIt
        self.moveit = MoveIt(self, 'arm')

    def move_to_pose(self, x, y, z):
        # Create target pose
        pose = PoseStamped()
        pose.header.frame_id = 'base_link'
        pose.pose.position.x = x
        pose.pose.position.y = y
        pose.pose.position.z = z
        pose.pose.orientation.w = 1.0

        # Plan and execute
        self.moveit.set_pose_target(pose)
        success = self.moveit.go()
        return success

def main():
    rclpy.init()
    planner = ArmPlanner()
    planner.move_to_pose(0.3, 0.0, 0.2)
    rclpy.shutdown()
```

### Plan and Execute from Command Line

```bash
# Move to named pose (defined in SRDF)
ros2 action send_goal /move_group moveit_msgs/action/MoveGroup \
  "{request: {group_name: 'arm', named_target: 'home'}}"

# Move gripper
ros2 action send_goal /gripper_controller/gripper_cmd control_msgs/action/GripperCommand \
  "{command: {position: 0.5, max_effort: 10.0}}"
```

---

## Integration with Hardware

### Controller Chain

```
MoveIt move_group
        │
        │ Publishes: FollowJointTrajectory goal
        ▼
/arm_controller/follow_joint_trajectory
        │
        │ Handled by: trajectory_bridge.py
        ▼
/arm/joint_commands (JointState)
        │
        │ Handled by: servo_bridge.py
        ▼
Arduino Serial: SET_ALL_SERVOS,90,45,60,90,90,90
        │
        ▼
Servo Motors (via PCA9685)
```

### Key Configuration for Hardware

In `moveit_controllers.yaml`, the action namespaces must match what `trajectory_bridge.py` provides:

```yaml
arm_controller:
  action_ns: follow_joint_trajectory  # → /arm_controller/follow_joint_trajectory

gripper_controller:
  action_ns: gripper_cmd              # → /gripper_controller/gripper_cmd
```

---

## Common Operations

### Move to Home Position

```bash
# Using MoveIt RViz plugin:
# 1. Select "arm" in Planning Group
# 2. Choose "home" from Goal State dropdown
# 3. Click Plan & Execute

# Or via command line - set joints to center positions
ros2 topic pub /arm/joint_commands sensor_msgs/JointState \
  "{name: ['joint_1','joint_2','joint_3','joint_4','gripper_base_joint','left_gear_joint'],
    position: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}"
```

### Open/Close Gripper

```bash
# Open gripper (0.8 radians)
ros2 action send_goal /gripper_controller/gripper_cmd control_msgs/action/GripperCommand \
  "{command: {position: 0.8}}"

# Close gripper (0.0 radians)
ros2 action send_goal /gripper_controller/gripper_cmd control_msgs/action/GripperCommand \
  "{command: {position: 0.0}}"
```

### Add Collision Object

```bash
# Add a box obstacle
ros2 service call /planning_scene moveit_msgs/srv/ApplyPlanningScene \
  "{scene: {world: {collision_objects: [{
      id: 'obstacle',
      header: {frame_id: 'base_link'},
      primitive_poses: [{position: {x: 0.3, y: 0.0, z: 0.1}}],
      primitives: [{type: 1, dimensions: [0.1, 0.1, 0.2]}]
  }]}}}"
```

---

## Troubleshooting

### Planning Fails (No Valid Solution)

1. **Check IK solver timeout**: Increase in `kinematics.yaml`
2. **Target may be unreachable**: Check workspace limits
3. **Collision detected**: Visualize planning scene in RViz

```bash
# View current planning scene
ros2 topic echo /planning_scene
```

### Controller Not Found

Verify action server is running:

```bash
ros2 action list | grep arm_controller
# Should show: /arm_controller/follow_joint_trajectory

ros2 action list | grep gripper_controller
# Should show: /gripper_controller/gripper_cmd
```

### Robot Not Moving

1. Check controller connection:
```bash
ros2 node info /trajectory_bridge
```

2. Verify joint commands are published:
```bash
ros2 topic echo /arm/joint_commands
```

3. Check servo_bridge serial connection:
```bash
ros2 topic echo /arduino_ack
```

### TF Errors

```bash
# Check TF tree
ros2 run tf2_tools view_frames

# Look for missing frames or old timestamps
ros2 topic echo /tf
```

---

## Modifying the Configuration

### Re-run Setup Assistant

```bash
ros2 launch arm_moveit setup_assistant.launch.py
```

### Add New Named Poses

Edit `mobile_arm_bot_2.srdf`:

```xml
<group_state name="ready" group="arm">
    <joint name="joint_1" value="0.0"/>
    <joint name="joint_2" value="-0.5"/>
    <joint name="joint_3" value="0.5"/>
    <joint name="joint_4" value="0.0"/>
    <joint name="gripper_base_joint" value="0.0"/>
</group_state>
```

### Change Planner

Edit move_group launch to use different planner:

```python
planning_plugin = "pilz_industrial_motion_planner/CommandPlanner"  # For linear motions
# or
planning_plugin = "ompl_interface/OMPLPlanner"  # Default, good for complex paths
```

---

## Dependencies

- `moveit_ros_move_group` - Move group node
- `moveit_kinematics` - IK solvers
- `moveit_planners` - Motion planners (OMPL, Pilz)
- `moveit_simple_controller_manager` - Controller interface
- `moveit_setup_assistant` - Configuration tool
- `arm_description` - Robot URDF

---

## Related Packages

- [arm_description](../arm_description/README.md) - Robot model
- [arm_controller](../arm_controller/README.md) - Hardware interface
- [pick_and_place](../../applications/pick_and_place/README.md) - Application using MoveIt
