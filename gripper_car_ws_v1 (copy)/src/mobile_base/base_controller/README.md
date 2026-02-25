# base_controller

ROS2 package providing hardware interface for 4-wheel differential drive mobile base with Arduino motor control.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Nodes](#nodes)
- [Differential Drive Kinematics](#differential-drive-kinematics)
- [Communication Protocol](#communication-protocol)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package bridges ROS2 navigation commands to physical motor hardware. It provides:

- **Velocity command processing** (`/cmd_vel` → motor speeds)
- **Encoder-based odometry** with TF broadcasting
- **Joint state publishing** for wheel visualization
- **Keyboard teleoperation** utility

### System Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│     Nav2 /      │     │ Base Hardware   │     │    Arduino      │
│    Teleop       │────▶│    Bridge       │────▶│   Motor Driver  │
│                 │     │                 │     │                 │
│   /cmd_vel      │     │ Serial Protocol │     │  L298N / etc    │
└─────────────────┘     └────────┬────────┘     └────────┬────────┘
                                 │                        │
                                 │◀───── Encoder ─────────┘
                                 │        Feedback
                        ┌────────▼────────┐
                        │ Publishes:      │
                        │  /odom          │
                        │  /base/joint_   │
                        │   states        │
                        │  TF: odom →     │
                        │      body_link  │
                        └─────────────────┘
```

---

## Package Structure

```
base_controller/
├── CMakeLists.txt              # Build configuration
├── package.xml                 # Package manifest
├── README.md                   # This documentation
├── scripts/
│   ├── base_hardware_bridge.py # Main motor controller node
│   ├── odom_publisher_node.py  # Standalone odometry (alternative)
│   ├── serial_bridge_node.py   # Low-level serial communication
│   └── teleop_keyboard.py      # Keyboard control utility
├── launch/
│   ├── base_control.launch.py  # Main launcher
│   ├── teleop.launch.py        # Teleop launcher
│   └── hardware.launch.py      # Hardware-only launcher
└── config/
    └── base_params.yaml        # Hardware parameters
```

---

## Nodes

### base_hardware_bridge.py

The primary node for motor control and odometry.

#### Purpose
- Converts ROS2 velocity commands to motor speeds
- Calculates odometry from encoder feedback
- Publishes wheel joint states for visualization
- Broadcasts TF transform (odom → body_link)

#### Topics

**Subscribed:**

| Topic | Type | Description |
|-------|------|-------------|
| `/cmd_vel` | `geometry_msgs/Twist` | Velocity commands (linear.x, angular.z) |

**Published:**

| Topic | Type | Description |
|-------|------|-------------|
| `/odom` | `nav_msgs/Odometry` | Robot odometry (pose + velocity) |
| `/base/joint_states` | `sensor_msgs/JointState` | Wheel positions for visualization |
| `/base/status` | `std_msgs/String` | Connection and error status |

**TF Broadcasts:**

| Parent | Child | Description |
|--------|-------|-------------|
| `odom` | `body_link` | Robot pose in odom frame |

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `serial_port` | string | `/dev/ttyACM0` | Arduino serial port |
| `baud_rate` | int | `115200` | Serial communication speed |
| `encoder_ticks_per_rev` | int | `360` | Encoder pulses per wheel rotation |
| `wheel_radius` | float | `0.075` | Wheel radius in meters |
| `wheel_base` | float | `0.67` | Distance between left/right wheels |
| `publish_rate` | float | `50.0` | Joint state publish rate (Hz) |
| `cmd_timeout` | float | `0.5` | Stop if no command received (seconds) |
| `base_frame` | string | `body_link` | Robot base TF frame |
| `odom_frame` | string | `odom` | Odometry TF frame |

---

### teleop_keyboard.py

Keyboard teleoperation for manual driving.

#### Controls

```
      w
 q    s    e
      a    d
 z    x    c
```

| Key | Action |
|-----|--------|
| `w` | Forward |
| `s` | Backward |
| `a` | Turn left |
| `d` | Turn right |
| `q` | Forward + left |
| `e` | Forward + right |
| `z` | Backward + left |
| `c` | Backward + right |
| `x` / `SPACE` | Stop |
| `+` / `-` | Increase/decrease linear speed |
| `[` / `]` | Increase/decrease angular speed |
| `ESC` | Quit |

#### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `linear_speed` | `0.3` | Initial forward speed (m/s) |
| `angular_speed` | `0.5` | Initial turn speed (rad/s) |
| `linear_step` | `0.05` | Speed increment |
| `angular_step` | `0.1` | Turn speed increment |
| `max_linear_speed` | `1.0` | Maximum linear velocity |
| `max_angular_speed` | `2.0` | Maximum angular velocity |

---

## Differential Drive Kinematics

### Understanding Differential Drive

A differential drive robot has two independently driven wheel sets (left and right). By varying the speed of each side, the robot can:

- **Go straight**: Left speed = Right speed
- **Turn in place**: Left speed = -Right speed
- **Arc turn**: Left speed ≠ Right speed (same sign)

### Forward Kinematics (Encoders → Odometry)

Given encoder counts, calculate robot motion:

```python
# Distance traveled by each wheel
left_dist = left_ticks * (2 * pi * wheel_radius / ticks_per_rev)
right_dist = right_ticks * (2 * pi * wheel_radius / ticks_per_rev)

# Robot motion
distance = (left_dist + right_dist) / 2.0  # Center distance
delta_theta = (right_dist - left_dist) / wheel_base  # Rotation

# Update pose (x, y, theta)
if abs(delta_theta) < epsilon:  # Straight line
    x += distance * cos(theta)
    y += distance * sin(theta)
else:  # Arc motion
    radius = distance / delta_theta
    x += radius * (sin(theta + delta_theta) - sin(theta))
    y -= radius * (cos(theta + delta_theta) - cos(theta))
    theta += delta_theta
```

### Inverse Kinematics (cmd_vel → Wheel Speeds)

Given desired linear and angular velocity, calculate wheel speeds:

```python
# From geometry_msgs/Twist
linear_x = cmd_vel.linear.x   # Forward velocity (m/s)
angular_z = cmd_vel.angular.z  # Turn rate (rad/s)

# Wheel velocities
v_left = linear_x - (angular_z * wheel_base / 2.0)
v_right = linear_x + (angular_z * wheel_base / 2.0)

# Convert to motor commands (depends on your motor driver)
```

### Visualization

```
                Forward (+linear_x)
                     ▲
                     │
            ┌────────┴────────┐
            │                 │
  ┌──┐      │     Robot      │      ┌──┐
  │L │      │                 │      │R │
  │W │      │        ●        │      │W │
  │  │      │   (body_link)   │      │  │
  └──┘      │                 │      └──┘
            │                 │
            └─────────────────┘

    ◄─────── wheel_base ───────►

  Turn Left: L slow, R fast
  Turn Right: L fast, R slow
  Rotate CCW: L backward, R forward
  Rotate CW: L forward, R backward
```

---

## Communication Protocol

### ROS2 → Arduino Commands

| Command | Format | Description |
|---------|--------|-------------|
| Velocity | `VEL,<linear>,<angular>` | Set desired velocities (m/s, rad/s) |
| Stop | `STOP` | Emergency stop all motors |
| Status | `STATUS` | Request current status |

**Examples:**
```bash
# Move forward at 0.5 m/s
echo "VEL,0.500,0.000" > /dev/ttyACM0

# Turn left in place
echo "VEL,0.000,0.500" > /dev/ttyACM0

# Stop
echo "STOP" > /dev/ttyACM0
```

### Arduino → ROS2 Messages

| Message | Format | Description |
|---------|--------|-------------|
| Odometry | `ODOM,<M1>,<M2>,<M3>,<M4>` | Encoder counts for 4 wheels |
| Status | `STATUS,<message>` | Status/info message |
| Error | `ERROR,<message>` | Error report |

**Encoder Mapping:**
- M1: Front Right
- M2: Front Left
- M3: Back Right
- M4: Back Left

---

## Usage

### Launch Base Controller

```bash
# With default parameters
ros2 launch base_controller base_control.launch.py

# With custom serial port
ros2 launch base_controller base_control.launch.py serial_port:=/dev/ttyUSB0

# With custom wheel parameters
ros2 launch base_controller base_control.launch.py \
  wheel_radius:=0.1 \
  wheel_base:=0.5
```

### Keyboard Teleoperation

```bash
# Launch teleop
ros2 launch base_controller teleop.launch.py

# Or run directly
ros2 run base_controller teleop_keyboard.py
```

### Send Velocity Commands

```bash
# Move forward at 0.2 m/s
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.2}, angular: {z: 0.0}}"

# Turn left
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.0}, angular: {z: 0.5}}"

# Arc turn (forward + left)
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.3}, angular: {z: 0.2}}"

# Stop
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.0}, angular: {z: 0.0}}"
```

### Monitor Odometry

```bash
# View odometry data
ros2 topic echo /odom

# Check TF tree
ros2 run tf2_tools view_frames

# Monitor odom → body_link transform
ros2 run tf2_ros tf2_echo odom body_link
```

### View Wheel States

```bash
ros2 topic echo /base/joint_states
```

---

## Configuration

### base_params.yaml

```yaml
base_hardware_bridge:
  ros__parameters:
    # Serial connection
    serial_port: "/dev/ttyACM0"
    baud_rate: 115200

    # Encoder configuration
    encoder_ticks_per_rev: 360

    # Physical dimensions
    wheel_radius: 0.075      # meters
    wheel_base: 0.67         # meters (left-right wheel spacing)

    # Publishing
    publish_rate: 50.0       # Hz

    # Safety
    cmd_timeout: 0.5         # seconds (stop if no cmd received)

    # TF frames
    base_frame: "body_link"
    odom_frame: "odom"
```

### Launch Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `serial_port` | `/dev/ttyACM0` | Arduino serial port |
| `baud_rate` | `115200` | Serial baud rate |
| `encoder_ticks_per_rev` | `360` | Encoder resolution |
| `wheel_radius` | `0.075` | Wheel radius (m) |
| `wheel_base` | `0.67` | Wheel separation (m) |
| `publish_rate` | `50.0` | State publish rate (Hz) |
| `base_frame` | `body_link` | Base TF frame |
| `odom_frame` | `odom` | Odometry frame |

---

## Understanding the Code

### base_hardware_bridge.py - Key Functions

#### `cmd_vel_callback(self, msg)`
Converts ROS2 Twist to Arduino command:

```python
def cmd_vel_callback(self, msg):
    linear_x = msg.linear.x
    angular_z = msg.angular.z

    # Send to Arduino (Arduino does inverse kinematics)
    command = f"VEL,{linear_x:.3f},{angular_z:.3f}"
    self.send_command(command)
```

#### `update_odometry(self, encoder_counts)`
Calculates pose from encoder feedback:

```python
def update_odometry(self, encoder_counts):
    # Calculate wheel distances (average left/right sides)
    left_ticks = (enc[0] + enc[2]) / 2.0 - (prev[0] + prev[2]) / 2.0
    right_ticks = (enc[1] + enc[3]) / 2.0 - (prev[1] + prev[3]) / 2.0

    left_dist = left_ticks * self.meters_per_tick
    right_dist = right_ticks * self.meters_per_tick

    # Differential drive kinematics
    distance = (left_dist + right_dist) / 2.0
    delta_theta = (right_dist - left_dist) / self.wheel_base

    # Update pose
    self.x += distance * cos(self.theta)
    self.y += distance * sin(self.theta)
    self.theta += delta_theta
```

#### `publish_odometry(self, timestamp, vx, vth)`
Publishes odometry message and TF:

```python
def publish_odometry(self, timestamp, vx, vth):
    # Create quaternion from yaw
    q = euler_to_quaternion(0, 0, self.theta)

    # Broadcast TF: odom → body_link
    t = TransformStamped()
    t.header.frame_id = "odom"
    t.child_frame_id = "body_link"
    t.transform.translation.x = self.x
    t.transform.translation.y = self.y
    t.transform.rotation = q
    self.tf_broadcaster.sendTransform(t)

    # Publish Odometry message
    odom = Odometry()
    odom.pose.pose.position.x = self.x
    odom.pose.pose.position.y = self.y
    odom.twist.twist.linear.x = vx
    odom.twist.twist.angular.z = vth
    self.odom_pub.publish(odom)
```

---

## Troubleshooting

### Serial Port Issues

```bash
# Check if port exists
ls -la /dev/ttyACM* /dev/ttyUSB*

# Check permissions
sudo chmod 666 /dev/ttyACM0

# Add user to dialout group (permanent fix)
sudo usermod -a -G dialout $USER
# Then log out and back in
```

### Robot Not Moving

1. Verify Arduino is receiving commands:
   - Open Arduino Serial Monitor
   - Send `VEL,0.5,0.0` manually
   - Check motor response

2. Check serial connection:
```bash
ros2 topic echo /base/status
```

3. Verify cmd_vel is being published:
```bash
ros2 topic echo /cmd_vel
```

### Odometry Drift

1. Calibrate wheel_radius:
   - Mark starting position
   - Drive exactly 1 meter
   - Compare actual vs. reported distance
   - Adjust wheel_radius accordingly

2. Calibrate wheel_base:
   - Rotate robot 360° in place
   - Measure actual vs. reported rotation
   - Adjust wheel_base accordingly

3. Check encoder counts:
```bash
# Monitor encoder feedback
ros2 topic echo /base/joint_states
```

### TF Issues

```bash
# Check TF tree
ros2 run tf2_tools view_frames

# Verify odom frame is being published
ros2 run tf2_ros tf2_echo odom body_link
```

---

## Dependencies

- `rclpy` - ROS2 Python client
- `geometry_msgs` - Twist, TransformStamped
- `nav_msgs` - Odometry
- `sensor_msgs` - JointState
- `std_msgs` - String
- `tf2_ros` - TF broadcaster
- `pyserial` - Serial communication

---

## Related Packages

- [mobile_base_description](../mobile_base_description/README.md) - Robot URDF model
- [mobile_base_gazebo](../mobile_base_gazebo/README.md) - Gazebo simulation
- [mobile_manipulator_navigation](../../mobile_manipulator/mobile_manipulator_navigation/README.md) - Nav2 configuration
