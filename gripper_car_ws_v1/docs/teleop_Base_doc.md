# COMPLETE STEP-BY-STEP: RUN & TEST BASE (ROS 2 + Arduino)

This is the single, clean procedure to run and test the robot base.
Follow top to bottom. Do not skip steps.

---

## PRE-CHECK (DO ONCE)

- Arduino connected to Raspberry Pi via USB
- Raspberry Pi powered ON
- PC and Raspberry Pi on the same network
- Workspace already built (bridge was running earlier)

If all good, continue.

---

## PART 1 — RASPBERRY PI (ROBOT SIDE)

### STEP 1: Login to Raspberry Pi (from PC)

ssh pi@192.168.1.27

Password:
123

---

### STEP 2: STOP any running bridge (MANDATORY)

pkill -9 -f serial_bridge
sudo fuser -k /dev/ttyACM0

This ensures:
- No duplicate bridge
- Serial port is free

---

### STEP 3: START the Serial Bridge (DO NOT CLOSE THIS TERMINAL)

source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash

export ROS_DOMAIN_ID=0
export ROS_LOCALHOST_ONLY=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

ros2 run base_controller serial_bridge_node.py

Expected output:
[INFO] Serial Bridge Node started
[INFO] Port: /dev/ttyACM0
[INFO] Baud: 115200
[INFO] Connected to /dev/ttyACM0

Leave this terminal OPEN.

---

## PART 2 — PC (CONTROL SIDE)

### STEP 4: Open a NEW terminal on PC

source /opt/ros/humble/setup.bash

export ROS_DOMAIN_ID=0
export ROS_LOCALHOST_ONLY=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

---

### STEP 5: Verify ROS communication

ros2 topic list

You MUST see:
- /cmd_vel
- /odom
- /encoder_ticks
- /base/status

If not, stop and recheck Part 1.

---

## PART 3 — TEST THE ROBOT

### OPTION A: Keyboard Teleop (RECOMMENDED)

ros2 run teleop_twist_keyboard teleop_twist_keyboard

Controls:
i  → forward
,  → backward
j  → left
l  → right
k  → stop
q  → increase speed
z  → decrease speed

If the robot moves, the system is working.

---

### OPTION B: Direct ROS Command Test (NO KEYBOARD)

Forward (3 seconds):
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.3}}" -r 10

Backward:
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: -0.3}}" -r 10

Turn Left:
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{angular: {z: 0.5}}" -r 10

Turn Right:
timeout 3 ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{angular: {z: -0.5}}" -r 10

STOP:
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}, angular: {z: 0.0}}"

---

## PART 4 — DEBUG / VERIFY (ONLY IF NEEDED)

Check velocity commands:
ros2 topic echo /cmd_vel

Check encoder feedback:
ros2 topic echo /encoder_ticks

Check odometry:
ros2 topic echo /odom

---

## RESTART FLOW (WHENEVER NEEDED)

On Raspberry Pi:

pkill -9 -f serial_bridge
sudo fuser -k /dev/ttyACM0

Then repeat from STEP 3 onward.

---

## FINAL RULES

- Never run two serial bridges
- Never close the Raspberry Pi bridge terminal
- Always set environment variables in every new terminal
- Always start the bridge before teleop
- /cmd_vel is the single source of truth

End of procedure.
