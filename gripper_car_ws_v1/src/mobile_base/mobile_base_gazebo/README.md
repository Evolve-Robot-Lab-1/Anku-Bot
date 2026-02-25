# mobile_base_gazebo

ROS2 package providing Gazebo simulation for the 4-wheel differential drive mobile base with simulated sensors.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Gazebo Plugins](#gazebo-plugins)
- [Simulated Sensors](#simulated-sensors)
- [World Files](#world-files)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package enables physics-based simulation of the mobile base in Gazebo. It includes:

- **Differential drive plugin** for motor simulation
- **LiDAR sensor** for navigation testing
- **RGB Camera** for vision applications
- **World files** for testing scenarios
- **Joint state publisher** for wheel visualization

### Why Simulate?

Simulation allows you to:
- Test navigation algorithms without hardware
- Develop and debug code safely
- Train machine learning models
- Validate URDF models before manufacturing

---

## Package Structure

```
mobile_base_gazebo/
├── CMakeLists.txt          # Build configuration
├── package.xml             # Package manifest
├── README.md               # This documentation
├── urdf/
│   └── gazebo.xacro        # Gazebo plugins and sensors
├── launch/
│   ├── gazebo.launch.py    # Main simulation launcher
│   └── gazebo_with_nav.launch.py  # Simulation + Nav2
├── config/
│   ├── sim.rviz            # RViz configuration
│   └── nav.rviz            # Navigation RViz config
└── worlds/
    ├── empty.world         # Empty world (default)
    ├── test_arena.world    # Testing arena
    └── qr_test_arena.world # QR code testing
```

---

## Gazebo Plugins

### Differential Drive Controller

Simulates motor control and odometry:

```xml
<plugin name="diff_drive" filename="libgazebo_ros_diff_drive.so">
  <!-- Wheels -->
  <left_joint>front_left_joint</left_joint>
  <right_joint>front_right_joint</right_joint>

  <!-- Physical parameters -->
  <wheel_separation>0.35</wheel_separation>  <!-- Distance between wheels -->
  <wheel_diameter>0.15</wheel_diameter>      <!-- 2 * wheel_radius -->

  <!-- Limits -->
  <max_wheel_torque>50</max_wheel_torque>
  <max_wheel_acceleration>2.0</max_wheel_acceleration>

  <!-- Topics -->
  <!-- Subscribes: /cmd_vel (Twist) -->
  <!-- Publishes: /odom (Odometry), TF: odom → body_link -->
</plugin>
```

**ROS2 Interface:**

| Topic | Type | Direction | Description |
|-------|------|-----------|-------------|
| `/cmd_vel` | `Twist` | Subscribe | Velocity commands |
| `/odom` | `Odometry` | Publish | Simulated odometry |

### Joint State Publisher

Publishes wheel joint positions for visualization:

```xml
<plugin name="joint_state_publisher" filename="libgazebo_ros_joint_state_publisher.so">
  <joint_name>front_left_joint</joint_name>
  <joint_name>front_right_joint</joint_name>
  <joint_name>back_left_joint</joint_name>
  <joint_name>back_right_joint</joint_name>
  <update_rate>50</update_rate>
</plugin>
```

**Topic:** `/joint_states` (sensor_msgs/JointState)

---

## Simulated Sensors

### LiDAR Sensor

360-degree 2D laser scanner simulating YDLiDAR:

```xml
<sensor type="ray" name="lidar_sensor">
  <update_rate>10</update_rate>
  <ray>
    <scan>
      <horizontal>
        <samples>360</samples>           <!-- Points per scan -->
        <min_angle>-3.14159</min_angle>  <!-- -180° -->
        <max_angle>3.14159</max_angle>   <!-- +180° -->
      </horizontal>
    </scan>
    <range>
      <min>0.12</min>   <!-- 12 cm minimum -->
      <max>12.0</max>   <!-- 12 m maximum -->
    </range>
  </ray>
</sensor>
```

| Property | Value | Description |
|----------|-------|-------------|
| Update Rate | 10 Hz | Scan frequency |
| Angular Resolution | 1° | 360 samples over 360° |
| Min Range | 0.12 m | Minimum detection distance |
| Max Range | 12.0 m | Maximum detection distance |
| Frame | `laser_frame` | TF frame for scan data |

**Topic:** `/scan` (sensor_msgs/LaserScan)

### RGB Camera

Front-facing camera for vision applications:

```xml
<sensor type="camera" name="camera_sensor">
  <update_rate>30.0</update_rate>
  <camera name="front_camera">
    <horizontal_fov>1.3962634</horizontal_fov>  <!-- ~80° -->
    <image>
      <width>640</width>
      <height>480</height>
      <format>R8G8B8</format>
    </image>
  </camera>
</sensor>
```

| Property | Value | Description |
|----------|-------|-------------|
| Resolution | 640x480 | VGA resolution |
| Update Rate | 30 Hz | Frame rate |
| FOV | ~80° | Horizontal field of view |
| Format | RGB8 | 8-bit color |
| Frame | `camera_link_optical` | TF frame (ROS convention) |

**Topics:**
- `/camera/image_raw` (sensor_msgs/Image)
- `/camera/camera_info` (sensor_msgs/CameraInfo)

---

## World Files

### empty.world

Minimal world with ground plane and lighting. Best for:
- Basic motion testing
- Odometry validation
- Quick debugging

### test_arena.world

Indoor arena with walls and obstacles:
- Enclosed space for navigation testing
- Walls for LiDAR reflection
- Suitable for SLAM testing

### qr_test_arena.world

Arena with QR code markers:
- For testing vision-based localization
- QR codes on walls for pick-and-place targets
- Fiducial marker detection testing

---

## Usage

### Basic Simulation

```bash
# Launch with empty world
ros2 launch mobile_base_gazebo gazebo.launch.py

# Launch with test arena
ros2 launch mobile_base_gazebo gazebo.launch.py world:=test_arena

# Launch without RViz
ros2 launch mobile_base_gazebo gazebo.launch.py use_rviz:=false
```

### Custom Spawn Position

```bash
# Spawn at specific location
ros2 launch mobile_base_gazebo gazebo.launch.py x:=1.0 y:=2.0 z:=0.1 yaw:=1.57

# Spawn at origin facing left (90°)
ros2 launch mobile_base_gazebo gazebo.launch.py yaw:=1.57
```

### Simulation with Navigation

```bash
# Launch simulation + Nav2
ros2 launch mobile_base_gazebo gazebo_with_nav.launch.py
```

### Drive the Robot

```bash
# Using teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Or publish velocity directly
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.2}, angular: {z: 0.1}}"
```

### View Sensor Data

```bash
# View LiDAR scan
ros2 topic echo /scan

# View camera image (requires image viewer)
ros2 run rqt_image_view rqt_image_view

# Check odometry
ros2 topic echo /odom
```

---

## Configuration

### Launch Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `use_rviz` | `true` | Launch RViz2 |
| `world` | `empty` | World file name |
| `x` | `0.0` | Initial X position |
| `y` | `0.0` | Initial Y position |
| `z` | `0.1` | Initial Z position |
| `yaw` | `0.0` | Initial yaw angle (radians) |

### Wheel Friction Parameters

In `gazebo.xacro`, adjust wheel friction for realistic behavior:

```xml
<gazebo reference="front_left_link">
  <mu1>1.5</mu1>        <!-- Friction coefficient 1 -->
  <mu2>1.5</mu2>        <!-- Friction coefficient 2 -->
  <kp>1000000.0</kp>    <!-- Stiffness -->
  <kd>100.0</kd>        <!-- Damping -->
</gazebo>
```

**Tuning Guide:**
- Higher `mu1/mu2` = more grip, less slipping
- Lower values = more realistic drift on smooth surfaces
- `kp/kd` affect contact dynamics

### LiDAR Configuration

Modify scan parameters in `gazebo.xacro`:

```xml
<scan>
  <horizontal>
    <samples>720</samples>  <!-- Increase for higher resolution -->
    <min_angle>-3.14159</min_angle>
    <max_angle>3.14159</max_angle>
  </horizontal>
</scan>
```

---

## Understanding gazebo.xacro

The `urdf/gazebo.xacro` file adds simulation-specific elements to the URDF:

### 1. Materials (Colors)

```xml
<gazebo reference="body_link">
  <material>Gazebo/Red</material>
</gazebo>
```

Gazebo uses its own material system, separate from URDF visual materials.

### 2. Friction Properties

```xml
<gazebo reference="front_left_link">
  <mu1>1.5</mu1>  <!-- Primary friction coefficient -->
  <mu2>1.5</mu2>  <!-- Secondary friction coefficient -->
</gazebo>
```

### 3. Plugins

Plugins add functionality not in the URDF:
- `libgazebo_ros_diff_drive.so` - Motor control
- `libgazebo_ros_ray_sensor.so` - LiDAR
- `libgazebo_ros_camera.so` - Camera
- `libgazebo_ros_joint_state_publisher.so` - Joint states

### How It's Included

The main URDF uses a conditional include:

```xml
<!-- In mobile_base.urdf.xacro -->
<xacro:if value="$(arg gazebo_plugins)">
  <xacro:include filename="$(arg gazebo_xacro_path)"/>
</xacro:if>
```

Launch file passes the argument:
```python
robot_description = Command([
    'xacro ', urdf_file,
    ' gazebo_plugins:=true',
    ' gazebo_xacro_path:=', gazebo_xacro
])
```

---

## Creating Custom Worlds

### Basic World Structure

```xml
<?xml version="1.0" ?>
<sdf version="1.6">
  <world name="my_world">
    <!-- Ground plane -->
    <include>
      <uri>model://ground_plane</uri>
    </include>

    <!-- Lighting -->
    <include>
      <uri>model://sun</uri>
    </include>

    <!-- Add your objects here -->
    <model name="box">
      <pose>2 0 0.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box><size>1 1 1</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>1 1 1</size></box>
          </geometry>
        </visual>
      </link>
    </model>
  </world>
</sdf>
```

Save as `worlds/my_world.world` and launch with:
```bash
ros2 launch mobile_base_gazebo gazebo.launch.py world:=my_world
```

---

## Troubleshooting

### Robot Falls Through Ground

1. Ensure robot spawns above ground (z > 0)
```bash
ros2 launch mobile_base_gazebo gazebo.launch.py z:=0.2
```

2. Check collision geometry is defined in URDF

### Robot Drifts When Stopped

1. Increase wheel friction in `gazebo.xacro`:
```xml
<mu1>2.0</mu1>
<mu2>2.0</mu2>
```

2. Check `max_wheel_torque` is sufficient

### Sensors Not Publishing

1. Verify topics exist:
```bash
ros2 topic list | grep -E "scan|camera|odom"
```

2. Check sensor frame exists:
```bash
ros2 run tf2_tools view_frames
```

3. Ensure Gazebo is running properly (check for error messages)

### Simulation Too Slow

1. Reduce sensor update rates in `gazebo.xacro`
2. Use simpler world (empty.world)
3. Disable visualization:
```xml
<visualize>false</visualize>
```

### TF Issues

1. Verify `use_sim_time` is set:
```bash
ros2 param get /robot_state_publisher use_sim_time
```

2. All nodes should use simulation time:
```python
parameters=[{'use_sim_time': True}]
```

---

## Dependencies

- `gazebo_ros` - Gazebo-ROS2 integration
- `gazebo_plugins` - Standard Gazebo plugins
- `robot_state_publisher` - URDF to TF
- `mobile_base_description` - Robot URDF
- `rviz2` - Visualization

---

## Related Packages

- [mobile_base_description](../mobile_base_description/README.md) - Robot URDF
- [base_controller](../base_controller/README.md) - Hardware interface (for real robot)
- [mobile_manipulator_sim](../../simulation/mobile_manipulator_sim/README.md) - Full robot simulation
