# Session Notes - 2025-12-28 (UPDATED)

## STATUS: UNIFIED CONTROL WORKING (Base + Arm)

## Hardware Setup
- **RPi**: 192.168.1.2, user: pi, password: 123
- **Arduino Mega**: Connected via /dev/ttyACM0 at 115200 baud
- **Workspace on RPi**: `/home/pi/pi_ros_ws/gripper_car_ws`
- **Workspace on PC**: `/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1`

## Motor Pin Configuration
| Motor | EN (PWM) | IN1 | IN2 |
|-------|----------|-----|-----|
| Front Left | 12 | 22 | 23 |
| Front Right | 11 | 24 | 25 |
| Back Left | 10 | 26 | 27 |
| Back Right | 9 | 28 | 29 |

## Arm Servo Configuration (PCA9685)
| Servo | Joint | Channel |
|-------|-------|---------|
| 0 | Base rotation (joint_1) | 0 |
| 1 | Shoulder (joint_2) | 1 |
| 2 | Elbow (joint_3) | 2 |
| 3 | Wrist (joint_4) | 3 |
| 4 | Gripper rotation (gripper_base_joint) | 4 |
| 5 | Gripper open/close (left_gear_joint) | 5 |

---

## Session 2025-12-28 Accomplishments

### 1. Arm Control via ROS (servo_bridge)
- Tested all 6 servos individually - ALL WORKING
- Command format: `SET_ALL_SERVOS,<s0>,<s1>,<s2>,<s3>,<s4>,<s5>`
- Values: 0-180 degrees

### 2. MoveIt Integration
- Installed MoveIt on PC: `sudo apt install ros-humble-moveit`
- Fixed `joint_limits.yaml` - changed `max_acceleration: 0.0` → `5.0`
- Created joint_state_relay (`/arm/joint_states` → `/joint_states`)
- MoveIt RViz drag-and-drop arm control WORKING

### 3. Unified Arduino Code
- Modified `mobile_manipulator_unified.ino` to accept both:
  - `ARM,<angles>` (original)
  - `SET_ALL_SERVOS,<angles>` (for servo_bridge compatibility)
- PCA9685 delay: 500ms after `pca.begin()` (was 100ms - caused servo init failure)
- Installed arduino-cli on RPi for remote uploads

### 4. Unified ROS Bridge
- Modified `serial_bridge_node.py` to handle BOTH base and arm
- Subscribes to: `/cmd_vel` (base), `/arm/joint_commands` (arm)
- Publishes: `/encoder_ticks`, `/arm/joint_states`, `/arduino_ack`
- Single serial port handles all communication

---

## Architecture

```
PC                                  RPi                         Arduino
├─ MoveIt (move_group)              ├─ serial_bridge_node.py    ├─ VEL → Motors
├─ RViz                             │   (unified bridge)        ├─ SET_ALL_SERVOS → PCA9685
├─ trajectory_bridge                │                           └─ ENC → Encoders
├─ joint_state_relay ──────────────►│
│  /arm/joint_states → /joint_states│
└─ teleop_twist_keyboard            │
```

---

## Quick Start - Unified Control

### Step 1: Launch Unified Bridge on RPi
```bash
sshpass -p '123' ssh pi@192.168.1.2 "
  pkill -9 -f python3 2>/dev/null; sleep 2
  nohup bash -c '
    source /opt/ros/humble/setup.bash && \
    source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash && \
    export ROS_DOMAIN_ID=0 && \
    export ROS_LOCALHOST_ONLY=0 && \
    export RMW_IMPLEMENTATION=rmw_fastrtps_cpp && \
    ros2 run base_controller serial_bridge_node.py
  ' > /tmp/unified_bridge.log 2>&1 &
  sleep 5
  tail -5 /tmp/unified_bridge.log
"
```

### Step 2: Setup PC Environment
```bash
export ROS_DOMAIN_ID=0
export ROS_LOCALHOST_ONLY=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
source /opt/ros/humble/setup.bash
source /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1/install/setup.bash
```

### Step 3: Control Base (cmd_vel)
```bash
# Forward
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.3}}" -r 10

# Turn
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{angular: {z: 0.5}}" -r 10

# Stop
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}}"

# Keyboard teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

### Step 4: Control Arm (Direct)
```bash
# Center all servos
ros2 topic pub --once /arduino_cmd std_msgs/msg/String "{data: 'SET_ALL_SERVOS,90,90,90,90,90,90'}"

# Move servo 0 (base) to 120
ros2 topic pub --once /arduino_cmd std_msgs/msg/String "{data: 'SET_ALL_SERVOS,120,90,90,90,90,90'}"
```

### Step 5: Control Arm (MoveIt)
```bash
# Start trajectory_bridge on PC
ros2 run arm_controller trajectory_bridge.py &

# Start joint_state_relay on PC
python3 -c "
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
class Relay(Node):
    def __init__(self):
        super().__init__('joint_state_relay')
        self.pub = self.create_publisher(JointState, '/joint_states', 10)
        self.sub = self.create_subscription(JointState, '/arm/joint_states', self.cb, 10)
    def cb(self, msg):
        self.pub.publish(msg)
rclpy.init()
rclpy.spin(Relay())
" &

# Launch MoveIt
ros2 launch arm_moveit demo.launch.py
```

---

## Arduino Serial Protocol (Unified)

### Commands (ROS → Arduino)
| Command | Format | Example |
|---------|--------|---------|
| Velocity | `VEL,<linear>,<angular>` | `VEL,0.25,0.0` |
| Arm (original) | `ARM,<a0>,...,<a5>` | `ARM,90,90,90,90,90,90` |
| Arm (servo_bridge) | `SET_ALL_SERVOS,<a0>,...,<a5>` | `SET_ALL_SERVOS,90,90,90,90,90,90` |
| Stop All | `STOP` | `STOP` |
| Stop Base | `STOP_BASE` | `STOP_BASE` |
| Reset Encoders | `RST` | `RST` |
| PID Tune | `PID,<kp>,<ki>,<kd>` | `PID,2.0,0.5,0.1` |

### Responses (Arduino → ROS)
| Response | Format |
|----------|--------|
| Encoders | `ENC,<fl>,<fr>,<bl>,<br>` |
| Arm ACK | `OK,SET_ALL_SERVOS` |
| Velocity ACK | `OK,VEL` |
| Status | `STATUS,<message>` |

---

## Issues Fixed This Session

1. **Arm servos not moving (unified code)**: PCA9685 delay was 100ms, needed 500ms
2. **MoveIt planning fails**: `max_acceleration: 0.0` in joint_limits.yaml → changed to 5.0
3. **MoveIt can't see joint states**: Created relay `/arm/joint_states` → `/joint_states`
4. **servo_bridge incompatible with unified code**: Added `SET_ALL_SERVOS` command support
5. **arduino-cli not on RPi**: Installed for remote firmware uploads

---

## Key Files Modified

### Arduino
- `arduino/mobile_manipulator_unified/mobile_manipulator_unified.ino`
  - Added `SET_ALL_SERVOS` command support (line 500-502)
  - PCA9685 delay 500ms (line 768)

### ROS2 (PC)
- `src/arm/arm_moveit/config/joint_limits.yaml`
  - Changed `max_acceleration: 0.0` → `5.0` for all joints

### ROS2 (RPi)
- `src/mobile_base/base_controller/scripts/serial_bridge_node.py`
  - Added arm control: `/arm/joint_commands` subscriber
  - Added arm feedback: `/arm/joint_states` publisher
  - Added `/arduino_ack` publisher

---

## TODO for Next Session
- [ ] Fix BL motor direction (swap wires 26/27) - if still backwards
- [ ] Create proper launch file for unified system (base + arm + MoveIt)
- [ ] Test odometry feedback loop
- [ ] Create web-based teleop
- [ ] Add collision objects to MoveIt scene

---

## LiDAR Integration Progress (2025-12-28)

### Hardware
- **Model**: YDLIDAR X2
- **Connection**: GPIO UART on RPi (`/dev/ttyAMA0`)
- **Wiring**: TX, RX, GND, VCC to RPi GPIO

### What Was Done
1. Updated LiDAR config port: `/dev/ttyUSB0` → `/dev/ttyAMA0`
2. **Disabled serial console** in `/boot/firmware/cmdline.txt`:
   - Removed `console=serial0,115200`
   - RPi rebooted for changes to take effect

### Config File
`src/ydlidar_ros2_driver/params/X2.yaml`:
```yaml
port: /dev/ttyAMA0
baudrate: 115200
isSingleChannel: true
range_max: 12.0
frequency: 10.0
```

### Next Steps to Test LiDAR
```bash
# 1. SSH to RPi
sshpass -p '123' ssh pi@192.168.1.2

# 2. Test LiDAR driver
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0 && export ROS_LOCALHOST_ONLY=0 && export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node --ros-args --params-file /home/pi/pi_ros_ws/gripper_car_ws/src/ydlidar_ros2_driver/params/X2.yaml

# 3. Check /scan topic from PC
ros2 topic echo /scan
```

### If LiDAR Still Fails
- Try swapping TX/RX wires
- Or use USB-to-Serial adapter

### Navigation Stack Ready
- SLAM: `ros2 launch robot_bringup slam.launch.py`
- Nav2: `ros2 launch robot_bringup navigation.launch.py`

---

## Troubleshooting

### Motors not moving
1. Check bridge is running: `ssh pi@192.168.1.2 "ps aux | grep serial_bridge"`
2. Check serial connected: `tail /tmp/unified_bridge.log`
3. Restart bridge and wait 5s for Arduino reset

### Arm not moving in MoveIt
1. Check trajectory_bridge running on PC
2. Check joint_state_relay running on PC
3. Try direct command: `ros2 topic pub --once /arduino_cmd ...`

### SSH connection fails
1. Check network: `ping 192.168.1.2`
2. Use longer timeout: `ssh -o ConnectTimeout=30`
3. Check WiFi on RPi
