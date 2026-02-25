# arm_description

ROS2 package containing the URDF/Xacro model and visual meshes for a 6-DOF robotic arm with a parallel-jaw gripper.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Robot Model](#robot-model)
- [Joint Configuration](#joint-configuration)
- [Usage](#usage)
- [Files Reference](#files-reference)
- [Customization](#customization)

---

## Overview

This package defines the kinematic and visual model of a 6-DOF (Degrees of Freedom) robotic arm designed for pick-and-place applications. The arm features:

- **6 revolute joints** for arm positioning
- **Parallel-jaw gripper** with mimic joints for synchronized finger movement
- **STL meshes** for realistic visualization in RViz and Gazebo
- **Inertial properties** for physics simulation

---

## Package Structure

```
arm_description/
├── CMakeLists.txt          # Build configuration
├── package.xml             # Package manifest
├── README.md               # This documentation
├── urdf/
│   ├── arm.urdf.xacro      # Main robot description (primary file)
│   ├── arm_macro.urdf.xacro # Parameterized macro for reuse
│   └── test.xacro          # Test configuration
├── launch/
│   └── display.launch.py   # RViz visualization launcher
├── config/
│   └── (rviz configs)      # RViz display settings
└── meshes/
    ├── base_link.STL       # Arm base mesh
    ├── link_1.STL          # Shoulder link
    ├── link_2.STL          # Upper arm
    ├── link_3.STL          # Forearm
    ├── link_4.STL          # Wrist
    ├── gripper_base_link.STL
    ├── left_gear_link.STL
    ├── right_gear_link.STL
    ├── left_finger_link.STL
    ├── right_finger_link.STL
    ├── left_link.STL
    └── right_link.STL
```

---

## Robot Model

### Kinematic Chain

```
base_link (fixed to robot body)
    │
    └── joint_1 (revolute, Z-axis) ──► link_1 [Base rotation]
            │
            └── joint_2 (revolute, Y-axis) ──► link_2 [Shoulder]
                    │
                    └── joint_3 (revolute, Y-axis) ──► link_3 [Elbow]
                            │
                            └── joint_4 (revolute, X-axis) ──► link_4 [Wrist pitch]
                                    │
                                    └── gripper_base_joint (revolute, Y-axis) ──► gripper_base_link [Wrist roll]
                                            │
                                            └── left_gear_joint (revolute, Z-axis) ──► left_gear_link [Gripper control]
                                                    │
                                                    ├── left_finger_joint (mimic)
                                                    ├── right_gear_joint (mimic)
                                                    ├── right_finger_joint (mimic)
                                                    ├── left_joint (mimic)
                                                    └── right_joint (mimic)
```

### Gripper Mechanism

The gripper uses a **master-slave mimic joint** system:

| Joint Name | Type | Multiplier | Description |
|------------|------|------------|-------------|
| `left_gear_joint` | Master | 1.0 | Controls gripper open/close |
| `right_gear_joint` | Mimic | -1.0 | Opposite motion for parallel grip |
| `left_finger_joint` | Mimic | 1.0 | Follows gear motion |
| `right_finger_joint` | Mimic | 1.0 | Follows gear motion |
| `left_joint` | Mimic | -1.0 | Linkage mechanism |
| `right_joint` | Mimic | 1.0 | Linkage mechanism |

**Why Mimic Joints?**
Mimic joints allow a single actuator (servo) to control multiple joints in a synchronized manner. When `left_gear_joint` moves, all mimic joints automatically follow according to their multiplier values.

---

## Joint Configuration

### Actuated Joints (6 DOF + Gripper)

| Joint | Axis | Lower Limit | Upper Limit | Description |
|-------|------|-------------|-------------|-------------|
| `joint_1` | Z | -180° (-π) | 180° (π) | Base rotation |
| `joint_2` | Y | -90° (-π/2) | 90° (π/2) | Shoulder pitch |
| `joint_3` | Y | -90° (-π/2) | 90° (π/2) | Elbow pitch |
| `joint_4` | X | -90° (-π/2) | 90° (π/2) | Wrist pitch |
| `gripper_base_joint` | Y | -180° (-π) | 180° (π) | Wrist roll |
| `left_gear_joint` | Z | 0 | 1.0 rad | Gripper open/close |

### Physical Properties

Each link includes:
- **Mass** (in kg)
- **Inertia tensor** (Ixx, Ixy, Ixz, Iyy, Iyz, Izz)
- **Visual mesh** (STL file)
- **Collision mesh** (same as visual)

---

## Usage

### View the Robot in RViz

```bash
# Build and source workspace
cd ~/gripper_car_ws
colcon build --packages-select arm_description
source install/setup.bash

# Launch visualization
ros2 launch arm_description display.launch.py
```

This launches:
- `robot_state_publisher` - Publishes TF transforms based on joint states
- `joint_state_publisher_gui` - Interactive sliders to move joints
- `rviz2` - 3D visualization

### Include in Another URDF

To include the arm in a larger robot model:

```xml
<?xml version="1.0"?>
<robot xmlns:xacro="http://www.ros.org/wiki/xacro" name="my_robot">

  <!-- Include arm model -->
  <xacro:include filename="$(find arm_description)/urdf/arm.urdf.xacro"/>

  <!-- Or use the macro for parameterized inclusion -->
  <xacro:include filename="$(find arm_description)/urdf/arm_macro.urdf.xacro"/>
  <xacro:arm_robot parent="your_mounting_link" xyz="0 0 0.1" rpy="0 0 0"/>

</robot>
```

---

## Files Reference

### urdf/arm.urdf.xacro

The main URDF file defining the complete arm. Key sections:

#### Xacro Macros

```xml
<!-- Generic link with mesh, inertial, visual, collision -->
<xacro:macro name="link_with_mesh" params="name mesh xyz rpy mass ixx ixy ixz iyy iyz izz">
  ...
</xacro:macro>

<!-- Revolute joint definition -->
<xacro:macro name="rev_joint" params="name parent child xyz rpy axis lower upper effort velocity">
  ...
</xacro:macro>

<!-- Revolute joint with mimic (for gripper mechanism) -->
<xacro:macro name="rev_joint_mimic" params="... mimic_joint multiplier offset">
  ...
</xacro:macro>
```

#### Key Properties

- **Mesh files**: Located in `package://arm_description/meshes/`
- **Coordinate frame**: Z-up, X-forward convention
- **Units**: Meters for length, radians for angles, kg for mass

### launch/display.launch.py

Visualization launch file that starts:

1. **robot_state_publisher**: Converts joint states to TF transforms
2. **joint_state_publisher_gui**: Provides interactive joint control sliders
3. **rviz2**: 3D visualization tool

```python
# Key nodes launched
Node(package='robot_state_publisher', executable='robot_state_publisher', ...)
Node(package='joint_state_publisher_gui', executable='joint_state_publisher_gui')
Node(package='rviz2', executable='rviz2')
```

---

## Customization

### Changing Joint Limits

Edit the joint definitions in `arm.urdf.xacro`:

```xml
<xacro:rev_joint name="joint_1"
                 parent="base_link" child="link_1"
                 ...
                 lower="-3.1416" upper="3.1416"  <!-- Modify these values -->
                 effort="10" velocity="2.0"/>
```

### Adding New Links

Use the provided macros:

```xml
<xacro:link_with_mesh
  name="new_link"
  mesh="package://arm_description/meshes/new_link.STL"
  xyz="0 0 0"
  rpy="0 0 0"
  mass="0.1"
  ixx="0.001" ixy="0" ixz="0"
  iyy="0.001" iyz="0" izz="0.001"/>
```

### Modifying Inertial Properties

For accurate simulation, calculate inertia tensors using CAD software or the [MeshLab](https://www.meshlab.net/) mesh processing tool.

---

## Dependencies

- `xacro` - URDF macro processor
- `robot_state_publisher` - Joint state to TF converter
- `joint_state_publisher` - Mock joint state publisher
- `rviz2` - Visualization
- `urdf` - URDF parser

---

## Troubleshooting

### Mesh not displaying in RViz
- Ensure `meshes/` folder is installed: check `CMakeLists.txt` includes `install(DIRECTORY meshes/ ...)`
- Verify mesh paths use `package://arm_description/meshes/` format

### TF frames not updating
- Confirm `robot_state_publisher` is running
- Check that `/joint_states` topic is being published
- Use `ros2 run tf2_tools view_frames` to visualize TF tree

### Collision detection issues
- Collision meshes use the same STL files as visual meshes
- For better performance, consider simplified collision geometry

---

## Related Packages

- [arm_controller](../arm_controller/README.md) - Hardware interface for servo control
- [arm_moveit](../arm_moveit/README.md) - MoveIt2 motion planning configuration
