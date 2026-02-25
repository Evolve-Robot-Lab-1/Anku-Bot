# Mobile Manipulator - Quick Start Guide
**Last Updated: 2025-12-27**

## Prerequisites
- RPi: 192.168.1.2 (user: pi, password: 123)
- PC and RPi on same network (192.168.1.x)
- Arduino connected to RPi via /dev/ttyACM0

---

## Step 1: Launch on RPi

### Option A: Via SSH from PC
```bash
sshpass -p '123' ssh pi@192.168.1.2 "
  source /opt/ros/humble/setup.bash && \
  source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash && \
  export ROS_DOMAIN_ID=0 && \
  export ROS_LOCALHOST_ONLY=0 && \
  export RMW_IMPLEMENTATION=rmw_fastrtps_cpp && \
  ros2 launch base_controller hardware.launch.py use_rviz:=false
"
```

### Option B: Directly on RPi
```bash
source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash
export ROS_DOMAIN_ID=0
export ROS_LOCALHOST_ONLY=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
ros2 launch base_controller hardware.launch.py use_rviz:=false
```

---

## Step 2: Control from PC

### Setup Environment (run once per terminal)
```bash
export ROS_DOMAIN_ID=0
export ROS_LOCALHOST_ONLY=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
source /opt/ros/humble/setup.bash
```

### Keyboard Teleop
```bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

**Controls:**
```
   u    i    o
   j    k    l
   m    ,    .

i = forward       , = backward
j = turn left     l = turn right
k = STOP
q = increase speed    z = decrease speed
```

---

## Manual Commands (Alternative to Teleop)

```bash
# Forward (3 seconds)
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.3}}" -r 10

# Backward (3 seconds)
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: -0.3}}" -r 10

# Turn Left (3 seconds)
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{angular: {z: 0.5}}" -r 10

# Turn Right (3 seconds)
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{angular: {z: -0.5}}" -r 10

# STOP
timeout 1 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}}" -r 10
```

---

## Emergency Stop

### Method 1: Send STOP via ROS
```bash
timeout 1 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}}" -r 10
```

### Method 2: Kill Processes (motors stop after 2s timeout)
```bash
sshpass -p '123' ssh pi@192.168.1.2 "killall -9 python3"
```

### Method 3: Direct Serial STOP
```bash
sshpass -p '123' ssh pi@192.168.1.2 "python3 << 'EOF'
import serial, time
s = serial.Serial('/dev/ttyACM0', 115200)
time.sleep(2.5)
s.write(b'STOP\n')
s.flush()
s.close()
print('STOP sent')
EOF"
```

---

## Troubleshooting

### Motors won't stop
1. Kill all processes: `sshpass -p '123' ssh pi@192.168.1.2 "killall -9 python3 ros2"`
2. Wait 3 seconds (Arduino timeout)
3. If still running, use direct serial STOP

### Can't see topics from PC
1. Verify both machines use same RMW: `export RMW_IMPLEMENTATION=rmw_fastrtps_cpp`
2. Restart ROS daemon: `ros2 daemon stop && ros2 daemon start`
3. Check network: `ping 192.168.1.2`

### Serial port locked
```bash
sshpass -p '123' ssh pi@192.168.1.2 "fuser -k /dev/ttyACM0; killall -9 python3"
```

### Stale publishers overriding commands
```bash
# On PC
pkill -f "ros2 topic pub"

# On RPi
sshpass -p '123' ssh pi@192.168.1.2 "pkill -f 'ros2 topic pub'"
```

---

## Arduino Firmware Upload (if needed)

Connect Arduino to PC via USB, then:
```bash
~/.local/bin/arduino-cli compile --fqbn arduino:avr:mega /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1/arduino/mobile_manipulator_unified

~/.local/bin/arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1/arduino/mobile_manipulator_unified
```

---

## Key Learnings

1. Always use `-r 10` instead of `--once` for reliable message delivery
2. Kill stale `ros2 topic pub` processes before testing
3. Arduino firmware must be re-uploaded after code changes
4. Both PC and RPi need `RMW_IMPLEMENTATION=rmw_fastrtps_cpp`
5. Motor auto-stops after 0.5s of no commands (serial bridge timeout)
6. Arduino auto-stops after 2s of no commands (firmware timeout)
