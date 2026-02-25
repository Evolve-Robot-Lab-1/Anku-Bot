# arm_controller

ROS2 package providing hardware interface bridges between ROS2 and servo motors controlled via Arduino/PCA9685 for the robotic arm.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [Nodes](#nodes)
- [Communication Protocol](#communication-protocol)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package bridges the gap between ROS2 motion planning (MoveIt2) and physical servo hardware. It provides:

- **Servo Bridge**: Converts ROS2 joint commands to Arduino serial commands
- **Trajectory Bridge**: Implements FollowJointTrajectory action server for MoveIt2 integration
- **Bidirectional Communication**: Receives joint position feedback from Arduino

### Architecture

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│      MoveIt2        │     │   Trajectory Bridge │     │    Servo Bridge     │
│  (Motion Planner)   │────▶│   (Action Server)   │────▶│  (Serial Interface) │
└─────────────────────┘     └─────────────────────┘     └──────────┬──────────┘
                                                                    │
                                                                    │ Serial (USB)
                                                                    │
                                                        ┌───────────▼───────────┐
                                                        │       Arduino         │
                                                        │    + PCA9685 PWM      │
                                                        └───────────┬───────────┘
                                                                    │
                                                        ┌───────────▼───────────┐
                                                        │    Servo Motors       │
                                                        │   (6 DOF + Gripper)   │
                                                        └───────────────────────┘
```

---

## Package Structure

```
arm_controller/
├── CMakeLists.txt              # Build configuration
├── package.xml                 # Package manifest
├── README.md                   # This documentation
├── scripts/
│   ├── servo_bridge.py         # Arduino serial communication node
│   └── trajectory_bridge.py    # MoveIt action server bridge
├── launch/
│   └── arm_control.launch.py   # Launch both bridge nodes
└── config/
    └── arm_params.yaml         # Hardware parameters
```

---

## Nodes

### servo_bridge.py

The core hardware interface node that communicates with the Arduino over serial.

#### Purpose
- Receives joint position commands from ROS2
- Converts radians to servo angles (0-180 degrees)
- Sends commands to Arduino via serial protocol
- Publishes current joint states based on Arduino feedback

#### Topics

**Subscribed:**

| Topic | Type | Description |
|-------|------|-------------|
| `/arm/joint_commands` | `sensor_msgs/JointState` | Target joint positions (radians) |
| `/arduino_cmd` | `std_msgs/String` | Raw Arduino commands (for debugging) |

**Published:**

| Topic | Type | Description |
|-------|------|-------------|
| `/arm/joint_states` | `sensor_msgs/JointState` | Current joint positions (radians) |
| `/arduino_ack` | `std_msgs/String` | Command acknowledgments from Arduino |
| `/arm/status` | `std_msgs/String` | Status messages (ARM_READY, etc.) |

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `serial_port` | string | `/dev/ttyACM0` | Arduino serial port |
| `baud_rate` | int | `115200` | Serial communication speed |
| `publish_rate` | float | `50.0` | Joint state publish rate (Hz) |
| `wait_for_ack` | bool | `true` | Wait for Arduino acknowledgment |
| `ack_timeout` | float | `2.0` | Acknowledgment timeout (seconds) |

#### Joint Mapping

The node handles 6 actuated joints plus mimic joints for the gripper:

```python
# Actuated joints (sent to Arduino)
joint_names = ['joint_1', 'joint_2', 'joint_3', 'joint_4',
               'gripper_base_joint', 'left_gear_joint']

# Mimic joints (calculated from left_gear_joint for TF publishing)
mimic_joints = {
    'right_gear_joint': -1.0,      # multiplier
    'left_finger_joint': 1.0,
    'right_finger_joint': 1.0,
    'left_joint': -1.0,
    'right_joint': 1.0,
}
```

#### Angle Conversion

Converts between ROS2 radians and servo degrees:

```
Servo Center (90°) ←→ Joint Center (midpoint of limits)
Servo 0°           ←→ Joint Lower Limit
Servo 180°         ←→ Joint Upper Limit

Special case: Gripper (left_gear_joint)
  Servo 90°  = Closed (0 rad)
  Servo 135° = Open (π/4 rad)
```

---

### trajectory_bridge.py

Provides ROS2 action servers for MoveIt2 trajectory execution.

#### Purpose
- Implements `FollowJointTrajectory` action for arm movements
- Implements `GripperCommand` action for gripper control
- Rate-limits commands for Arduino compatibility
- Publishes progress feedback during execution

#### Action Servers

| Action | Type | Description |
|--------|------|-------------|
| `/arm_controller/follow_joint_trajectory` | `FollowJointTrajectory` | Arm trajectory execution |
| `/gripper_controller/gripper_cmd` | `GripperCommand` | Gripper open/close |

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `min_command_interval` | float | `1.0` | Minimum seconds between commands |
| `skip_intermediate_points` | bool | `true` | Skip to final trajectory point |

#### Rate Limiting

Arduino servos cannot process rapid trajectory points. The bridge implements:

1. **Command Interval**: Enforces minimum time between serial commands
2. **Point Skipping**: Optionally sends only start and end points of trajectory
3. **Acknowledgment Waiting**: Waits for Arduino to confirm command completion

```python
# Example: 100-point trajectory is reduced to 2 points
# Point 1 (start) → wait 1 second → Point 100 (end)
```

---

## Communication Protocol

### ROS2 → Arduino Commands

| Command | Format | Description |
|---------|--------|-------------|
| Set all servos | `SET_ALL_SERVOS,a1,a2,a3,a4,a5,a6` | Set all 6 servo angles (0-180°) |
| Set single servo | `SERVO,<id>,<angle>` | Set one servo angle |

### Arduino → ROS2 Messages

| Message | Format | Description |
|---------|--------|-------------|
| Position feedback | `SERVO_POS,a1,a2,a3,a4,a5,a6` | Current servo positions |
| Acknowledgment | `OK,SET_ALL_SERVOS` | Command completed |
| Error | `ERROR,<description>` | Command failed |
| Heartbeat | `HEARTBEAT <millis>` | Arduino alive signal |
| Ready | `ARM READY` | Startup complete |

---

## Usage

### Launch Arm Controller

```bash
# With default serial port
ros2 launch arm_controller arm_control.launch.py

# With custom serial port
ros2 launch arm_controller arm_control.launch.py serial_port:=/dev/ttyUSB0

# Without trajectory bridge (servo bridge only)
ros2 launch arm_controller arm_control.launch.py use_trajectory_bridge:=false
```

### Send Direct Joint Commands

```bash
# Move joint_1 to 0.5 radians
ros2 topic pub /arm/joint_commands sensor_msgs/JointState \
  "{name: ['joint_1'], position: [0.5]}"

# Open gripper
ros2 topic pub /arm/joint_commands sensor_msgs/JointState \
  "{name: ['left_gear_joint'], position: [0.8]}"

# Close gripper
ros2 topic pub /arm/joint_commands sensor_msgs/JointState \
  "{name: ['left_gear_joint'], position: [0.0]}"
```

### Send Raw Arduino Commands

```bash
# Move servo 0 to 90 degrees
ros2 topic pub /arduino_cmd std_msgs/String "data: 'SERVO,0,90'"

# Set all servos to home position
ros2 topic pub /arduino_cmd std_msgs/String "data: 'SET_ALL_SERVOS,90,90,90,90,90,90'"
```

### Monitor Joint States

```bash
# View current joint positions
ros2 topic echo /arm/joint_states

# Check publishing rate
ros2 topic hz /arm/joint_states
```

---

## Configuration

### arm_params.yaml

```yaml
servo_bridge:
  ros__parameters:
    serial_port: "/dev/ttyACM0"
    baud_rate: 115200
    publish_rate: 50.0
    wait_for_ack: true
    ack_timeout: 2.0

trajectory_bridge:
  ros__parameters:
    min_command_interval: 1.0
    skip_intermediate_points: true
```

### Launch File Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `serial_port` | `/dev/ttyACM0` | Arduino serial port |
| `baud_rate` | `115200` | Serial baud rate |
| `publish_rate` | `50.0` | Joint state publish frequency |
| `use_trajectory_bridge` | `true` | Enable MoveIt integration |

---

## Understanding the Code

### servo_bridge.py - Key Functions

#### `joint_cmd_callback(self, msg)`
Handles incoming joint commands from trajectory_bridge or direct publishing:

```python
def joint_cmd_callback(self, msg):
    # 1. Wait for previous command acknowledgment (if enabled)
    if self.wait_for_ack and self.pending_ack:
        self.ack_received.wait(timeout=self.ack_timeout)

    # 2. Convert each joint position from radians to servo degrees
    for joint_name in msg.name:
        idx = self.joint_names.index(joint_name)
        angle_deg = self.joint_to_servo_angle(idx, msg.position[i])
        servo_angles[idx] = angle_deg

    # 3. Send command to Arduino
    command = f"SET_ALL_SERVOS,{','.join(servo_angles)}"
    self.send_command(command)
```

#### `joint_to_servo_angle(self, joint_idx, position_rad)`
Converts ROS2 joint positions (radians) to servo angles (degrees):

```python
def joint_to_servo_angle(self, joint_idx, position_rad):
    # Get joint limits from URDF
    lower, upper = self.joint_limits[joint_name]

    # Map joint range to servo range [0, 180]
    # Center of joint range → 90°
    joint_range = upper - lower
    center = (upper + lower) / 2.0
    angle = 90.0 + ((position_rad - center) / (joint_range / 2.0)) * 90.0

    return int(np.clip(angle, 0, 180))
```

### trajectory_bridge.py - Key Functions

#### `execute_trajectory(self, goal_handle)`
Executes MoveIt trajectories with rate limiting:

```python
async def execute_trajectory(self, goal_handle):
    trajectory = goal_handle.request.trajectory

    # Skip intermediate points if enabled
    if self.skip_intermediate and len(trajectory.points) > 2:
        points_to_send = [0, len(trajectory.points) - 1]  # First and last only

    for point in trajectory.points:
        # Rate limiting
        time.sleep(self.min_command_interval)

        # Publish joint command
        joint_cmd = JointState()
        joint_cmd.name = trajectory.joint_names
        joint_cmd.position = point.positions
        self.joint_cmd_pub.publish(joint_cmd)

    goal_handle.succeed()
```

---

## Troubleshooting

### Serial Port Issues

```bash
# List USB devices
ls -la /dev/ttyACM* /dev/ttyUSB*

# Check permissions
sudo chmod 666 /dev/ttyACM0

# Add user to dialout group (permanent fix)
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Arduino Not Responding

1. Check if Arduino is receiving data:
   - Open Arduino Serial Monitor
   - Verify baud rate matches (115200)

2. Test serial connection:
```bash
# Send test command
ros2 topic pub /arduino_cmd std_msgs/String "data: 'STATUS'"

# Check for response
ros2 topic echo /arduino_ack
```

### Joint States Not Publishing

1. Verify servo_bridge is running:
```bash
ros2 node list | grep servo_bridge
```

2. Check topic:
```bash
ros2 topic info /arm/joint_states
```

3. Verify joint_state_aggregator is combining topics (if using full system):
```bash
ros2 topic echo /joint_states
```

### MoveIt Not Executing Trajectories

1. Verify trajectory_bridge is running:
```bash
ros2 node list | grep trajectory_bridge
```

2. Check action server:
```bash
ros2 action list | grep arm_controller
```

3. Monitor action feedback:
```bash
ros2 action info /arm_controller/follow_joint_trajectory
```

---

## Dependencies

- `rclpy` - ROS2 Python client
- `sensor_msgs` - JointState messages
- `std_msgs` - String messages
- `control_msgs` - FollowJointTrajectory, GripperCommand actions
- `trajectory_msgs` - JointTrajectory messages
- `pyserial` - Serial communication (`pip3 install pyserial`)
- `numpy` - Numerical operations

---

## Related Packages

- [arm_description](../arm_description/README.md) - Robot URDF model
- [arm_moveit](../arm_moveit/README.md) - MoveIt2 configuration
- [mobile_manipulator_hardware](../../mobile_manipulator/mobile_manipulator_hardware/README.md) - Unified hardware interface
