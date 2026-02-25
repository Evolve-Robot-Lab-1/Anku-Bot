# Arm Hardware Testing - Session 2026-01-04

## Summary

Tested 6-DOF robotic arm with unified Arduino firmware (base + arm on single Mega 2560).

## Hardware Status

| Servo | Channel | Joint | Status |
|-------|---------|-------|--------|
| 0 | CH0 | Base rotation | ✅ Working |
| 1 | CH1 | Shoulder | ✅ Working |
| 2 | CH2 | Elbow | ✅ Working (fixed wiring) |
| 3 | CH4 | Wrist | ✅ Working |
| 4 | CH5 | Gripper rotation | ✅ Working |
| 5 | CH6 | Gripper | ✅ Working |

## Architecture

```
Local PC (192.168.1.14)          RPi (192.168.1.7)
┌─────────────────────┐          ┌─────────────────────┐
│ MoveIt + RViz       │          │ servo_bridge.py     │
│ trajectory_bridge   │──ROS2───▶│ (optional)          │
│ joint_state_relay   │          │                     │
└─────────────────────┘          │ Arduino Mega 2560   │
                                 │ /dev/ttyACM0        │
                                 │ - PCA9685 (arm)     │
                                 │ - L298N (base)      │
                                 └─────────────────────┘
```

## Quick Test Commands

### Direct Serial Test (on RPi)
```bash
ssh pi@192.168.1.7

python3 << 'EOF'
import serial, time
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)
time.sleep(2)

# Test all servos
ser.write(b'ARM,45,60,120,100,70,50\n')
time.sleep(3)
ser.write(b'ARM,90,90,90,90,90,90\n')
time.sleep(2)

# Test single servo
ser.write(b'SERVO,0,60\n')  # Move servo 0 to 60 degrees
time.sleep(2)
ser.write(b'SERVO,0,90\n')  # Return to center

ser.close()
EOF
```

### Arduino Commands

| Command | Description |
|---------|-------------|
| `ARM,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>` | Set all 6 servo angles (0-180) |
| `SERVO,<idx>,<angle>` | Set single servo |
| `VEL,<linear>,<angular>` | Base velocity |
| `STOP` | Emergency stop all |
| `STOP_ARM` | Stop arm only |
| `STATUS` | Get status |

### Servo Channel Mapping

```cpp
const uint8_t SERVO_CH[6] = {0, 1, 8, 4, 5, 6};  // Updated: Servo 2 on CH8
// Servo 0 (base)     -> PCA9685 channel 0
// Servo 1 (shoulder) -> PCA9685 channel 1
// Servo 2 (elbow)    -> PCA9685 channel 8  (was CH2, moved to CH8!)
// Servo 3 (wrist)    -> PCA9685 channel 4
// Servo 4 (grip_rot) -> PCA9685 channel 5
// Servo 5 (gripper)  -> PCA9685 channel 6
```

## MoveIt Control

### Start Components
```bash
# On RPi - start servo bridge
ssh pi@192.168.1.7
python3 ~/servo_bridge.py --ros-args -p serial_port:=/dev/ttyACM0

# On Local PC
source /opt/ros/humble/setup.bash && source install/setup.bash

# Start trajectory bridge
python3 src/arm/arm_controller/scripts/trajectory_bridge.py &

# Start joint state relay (bridges /arm/joint_states to /joint_states)
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

# Launch MoveIt + RViz
ros2 launch arm_moveit demo.launch.py
```

### ROS Topics

| Topic | Type | Description |
|-------|------|-------------|
| `/arm/joint_commands` | JointState | Commands to servo_bridge |
| `/arm/joint_states` | JointState | Feedback from servo_bridge |
| `/joint_states` | JointState | Relayed for MoveIt |

## Firmware Location

- Unified firmware: `arduino/mobile_manipulator_unified_obstacle/`
- Uploaded via: `arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega`

## Issues Fixed

1. **Servo 2 not working** - Fixed by checking wiring on PCA9685 channel 2
2. **MoveIt not executing** - Fixed by adding joint_state_relay to bridge topics

## Next Steps

- [ ] Test MoveIt Plan & Execute with real arm
- [ ] Test combined arm + base + obstacle avoidance
- [ ] Calibrate servo angles to match URDF joint limits

---
Session Date: 2026-01-04
