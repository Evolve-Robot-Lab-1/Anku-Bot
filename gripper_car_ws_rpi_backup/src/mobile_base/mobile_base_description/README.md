# mobile_base_description

ROS2 package containing the URDF/Xacro model for a 4-wheel differential drive mobile platform with sensor mounts.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Robot Model](#robot-model)
- [Parameters](#parameters)
- [Usage](#usage)
- [Integration](#integration)
- [Customization](#customization)

---

## Overview

This package defines the kinematic and visual model of a 4-wheel differential drive mobile base designed for indoor navigation and manipulation tasks. The platform includes:

- **4 independently driven wheels** for differential drive locomotion
- **LiDAR mount** for 2D laser scanning (YDLiDAR compatible)
- **Camera mount** for vision-based applications
- **Arm mounting point** for robotic arm attachment
- **ROS2 Control interfaces** for hardware integration

---

## Package Structure

```
mobile_base_description/
├── CMakeLists.txt          # Build configuration
├── package.xml             # Package manifest
├── README.md               # This documentation
├── urdf/
│   └── mobile_base.urdf.xacro    # Main robot description
├── launch/
│   └── display.launch.py         # RViz visualization
├── config/
│   └── (rviz configurations)
└── meshes/
    └── (optional 3D meshes)
```

---

## Robot Model

### Physical Dimensions

| Component | Dimension | Value |
|-----------|-----------|-------|
| Chassis Length | X | 0.5 m |
| Chassis Width | Y | 0.3 m |
| Chassis Height | Z | 0.15 m |
| Chassis Mass | - | 20.0 kg |
| Wheel Radius | - | 0.075 m (75 mm) |
| Wheel Width | - | 0.05 m (50 mm) |
| Wheel Mass | - | 2.5 kg each |
| Wheel Base | - | 0.35 m (left-right) |
| Wheel Track | - | 0.30 m (front-back) |

### Coordinate System

```
                  +X (Forward)
                     ▲
                     │
                     │
          +Y ◄───────┼─────────
         (Left)      │ body_link
                     │
                     ▼

        Wheels:
        FL ────── FR    (Front)
         │        │
         │   ●    │     (● = body_link origin)
         │        │
        BL ────── BR    (Back)
```

### Link Tree

```
body_link (chassis)
    ├── front_right_joint (continuous) ──► front_right_link (wheel)
    ├── front_left_joint (continuous) ──► front_left_link (wheel)
    ├── back_right_joint (continuous) ──► back_right_link (wheel)
    ├── back_left_joint (continuous) ──► back_left_link (wheel)
    ├── camera_joint (fixed) ──► camera_link ──► camera_link_optical
    └── arm_mount_joint (fixed) ──► arm_mount_link
```

### Wheel Configuration

The robot uses a **4-wheel differential drive** configuration:

| Wheel | Joint Name | Position (x, y, z) |
|-------|------------|--------------------|
| Front Right | `front_right_joint` | (0.15, -0.175, -0.07) |
| Front Left | `front_left_joint` | (0.15, 0.175, -0.07) |
| Back Right | `back_right_joint` | (-0.15, -0.175, -0.07) |
| Back Left | `back_left_joint` | (-0.15, 0.175, -0.07) |

**Differential Drive Kinematics**:
- Left wheels (FL, BL) rotate together
- Right wheels (FR, BR) rotate together
- Same speed = straight motion
- Different speeds = turning

---

## Parameters

### Xacro Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `gazebo_plugins` | `false` | Include Gazebo-specific plugins |
| `gazebo_xacro_path` | `""` | Path to Gazebo xacro file |
| `sim_mode` | `false` | Skip ros2_control blocks for simulation |

### Xacro Properties (Constants)

```xml
<!-- Chassis dimensions -->
<xacro:property name="chassis_length" value="0.5"/>
<xacro:property name="chassis_width" value="0.3"/>
<xacro:property name="chassis_height" value="0.15"/>
<xacro:property name="chassis_mass" value="20.0"/>

<!-- Wheel properties -->
<xacro:property name="wheel_radius" value="0.075"/>
<xacro:property name="wheel_width" value="0.05"/>
<xacro:property name="wheel_mass" value="2.5"/>
<xacro:property name="wheel_offset_x" value="0.15"/>
<xacro:property name="wheel_offset_y" value="0.175"/>
<xacro:property name="wheel_offset_z" value="-0.07"/>
```

---

## Usage

### View Robot in RViz

```bash
# Build and source
cd ~/gripper_car_ws
colcon build --packages-select mobile_base_description
source install/setup.bash

# Launch visualization
ros2 launch mobile_base_description display.launch.py
```

### Include in Another URDF

```xml
<?xml version="1.0"?>
<robot xmlns:xacro="http://www.ros.org/wiki/xacro" name="my_robot">

  <!-- Include mobile base -->
  <xacro:include filename="$(find mobile_base_description)/urdf/mobile_base.urdf.xacro"/>

</robot>
```

### Generate URDF from Xacro

```bash
# Standard URDF (for hardware)
xacro src/mobile_base/mobile_base_description/urdf/mobile_base.urdf.xacro > robot.urdf

# Simulation mode (excludes ros2_control)
xacro src/mobile_base/mobile_base_description/urdf/mobile_base.urdf.xacro sim_mode:=true > robot_sim.urdf

# With Gazebo plugins
xacro src/mobile_base/mobile_base_description/urdf/mobile_base.urdf.xacro \
  gazebo_plugins:=true \
  gazebo_xacro_path:='$(find mobile_base_gazebo)/urdf/gazebo.xacro' > robot_gazebo.urdf
```

---

## Integration

### ROS2 Control Configuration

The URDF includes ros2_control blocks for hardware integration (excluded in `sim_mode`):

```xml
<ros2_control name="mobile_base_system" type="system">
  <hardware>
    <plugin>base_controller/BaseHardwareInterface</plugin>
    <param name="serial_port">/dev/ttyACM0</param>
    <param name="wheel_radius">0.075</param>
    <param name="wheel_base">0.35</param>
    <param name="encoder_cpr">360</param>
  </hardware>

  <joint name="front_left_joint">
    <command_interface name="velocity"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
  <!-- ... other wheel joints ... -->
</ros2_control>
```

### Sensor Mounts

**Camera Mount** (`camera_link`):
- Position: Back of chassis, center
- Orientation: Looking backward (adjust for your camera)
- Optical frame: `camera_link_optical` with standard ROS camera convention

**Arm Mount** (`arm_mount_link`):
- Position: Top of chassis, forward center (0.15, 0, chassis_height/2)
- Used as parent link when attaching robotic arm

---

## Customization

### Changing Dimensions

Edit the xacro properties at the top of `mobile_base.urdf.xacro`:

```xml
<!-- Example: Larger chassis -->
<xacro:property name="chassis_length" value="0.7"/>
<xacro:property name="chassis_width" value="0.4"/>

<!-- Example: Bigger wheels -->
<xacro:property name="wheel_radius" value="0.10"/>
```

### Adding a LiDAR Mount

The LiDAR is typically added by the combined URDF (mobile_manipulator_description) or the driver package. To add directly:

```xml
<joint name="lidar_joint" type="fixed">
  <parent link="body_link"/>
  <child link="lidar_link"/>
  <origin xyz="0 0 ${chassis_height/2 + 0.05}" rpy="0 0 0"/>
</joint>

<link name="lidar_link">
  <visual>
    <geometry>
      <cylinder radius="0.05" length="0.06"/>
    </geometry>
  </visual>
</link>
```

### Using Custom Meshes

Replace primitive shapes with STL/DAE meshes:

```xml
<visual>
  <geometry>
    <!-- Instead of: <box size="0.5 0.3 0.15"/> -->
    <mesh filename="package://mobile_base_description/meshes/chassis.stl"/>
  </geometry>
</visual>
```

---

## Understanding the Code

### Wheel Macro

The URDF uses a macro to define all four wheels identically:

```xml
<xacro:macro name="wheel" params="name reflect_y reflect_x">
  <joint name="${name}_joint" type="continuous">
    <origin xyz="${reflect_x*wheel_offset_x} ${reflect_y*wheel_offset_y} ${wheel_offset_z}"/>
    <axis xyz="0 1 0"/>  <!-- Rotation around Y-axis -->
  </joint>

  <link name="${name}_link">
    <!-- Cylinder rotated to roll around Y-axis -->
    <visual>
      <origin rpy="${pi/2} 0 0"/>  <!-- Rotate cylinder to align with Y -->
      <geometry>
        <cylinder radius="${wheel_radius}" length="${wheel_width}"/>
      </geometry>
    </visual>
  </link>
</xacro:macro>

<!-- Instantiate 4 wheels with different positions -->
<xacro:wheel name="front_right" reflect_y="-1" reflect_x="1"/>
<xacro:wheel name="front_left" reflect_y="1" reflect_x="1"/>
<xacro:wheel name="back_right" reflect_y="-1" reflect_x="-1"/>
<xacro:wheel name="back_left" reflect_y="1" reflect_x="-1"/>
```

### Inertia Calculations

Proper inertia tensors are crucial for physics simulation:

```xml
<!-- Box inertia macro -->
<xacro:macro name="box_inertia" params="m x y z">
  <inertia ixx="${m*(y*y+z*z)/12.0}"
           iyy="${m*(x*x+z*z)/12.0}"
           izz="${m*(x*x+y*y)/12.0}"
           ixy="0.0" ixz="0.0" iyz="0.0"/>
</xacro:macro>

<!-- Cylinder inertia macro -->
<xacro:macro name="cylinder_inertia" params="m r h">
  <inertia ixx="${m*(3*r*r+h*h)/12.0}"
           iyy="${m*(3*r*r+h*h)/12.0}"
           izz="${m*r*r/2.0}"
           ixy="0.0" ixz="0.0" iyz="0.0"/>
</xacro:macro>
```

---

## Dependencies

- `xacro` - URDF macro processor
- `robot_state_publisher` - Joint state to TF converter
- `joint_state_publisher` - Mock joint state publisher
- `rviz2` - Visualization
- `urdf` - URDF parser

---

## Troubleshooting

### Wheels not rotating in RViz
- Ensure `/base/joint_states` topic is being published
- Check joint names match between URDF and publisher

### Robot floating in Gazebo
- Verify inertial properties are reasonable
- Check collision geometry is defined
- Ensure ground plane exists in world

### Camera frame issues
- Use `camera_link_optical` for vision algorithms (follows ROS camera convention)
- Use `camera_link` for physical mounting reference

---

## Related Packages

- [base_controller](../base_controller/README.md) - Motor control and odometry
- [mobile_base_gazebo](../mobile_base_gazebo/README.md) - Gazebo simulation
- [mobile_manipulator_description](../../mobile_manipulator/mobile_manipulator_description/README.md) - Combined robot model
