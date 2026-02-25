# Gripper Car Workspace V1 - Project Overview

## Executive Summary

A **ROS2 Humble** mobile manipulator project combining a 4-wheel differential drive base with a 6-DOF robotic arm and gripper. The system supports autonomous navigation (Nav2), SLAM mapping, motion planning (MoveIt2), and pick-and-place applications with QR code detection.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                            │
├─────────────────────────────────────────────────────────────────────┤
│  Pick-and-Place    │   MoveIt2 Motion    │   Nav2 Autonomous       │
│  State Machine     │   Planning          │   Navigation            │
└─────────────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────────────┐
│                     ROS2 MIDDLEWARE (DDS)                           │
├─────────────────────────────────────────────────────────────────────┤
│  Topics: /cmd_vel, /odom, /scan, /joint_states, /tf                 │
│  Services: /move_group/plan, /qr_scanner/detect                     │
│  Actions: /navigate_to_pose, /follow_joint_trajectory               │
└─────────────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────────────┐
│                  HARDWARE ABSTRACTION LAYER                         │
├─────────────────────────────────────────────────────────────────────┤
│  servo_bridge.py    │  base_hardware_bridge.py  │  ydlidar_driver   │
│  (Arm Control)      │  (Base Control)           │  (LiDAR)          │
└─────────────────────────────────────────────────────────────────────┘
                                │
┌─────────────────────────────────────────────────────────────────────┐
│                      PHYSICAL HARDWARE                              │
├─────────────────────────────────────────────────────────────────────┤
│  Arduino Mega 2560  ──► PCA9685 (6 Arm Servos)                      │
│                     ──► 2x L298N (4 DC Motors)                      │
│  YDLiDAR X2L        ──► /dev/ttyUSB0                                │
│  RPi Camera v2      ──► CSI Interface                               │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
gripper_car_ws_v1/
├── src/
│   ├── arm/                          # Robotic arm subsystem
│   │   ├── arm_controller/           # Arduino/servo bridge
│   │   ├── arm_description/          # URDF/meshes
│   │   └── arm_moveit/               # MoveIt2 config
│   │
│   ├── mobile_base/                  # Mobile base subsystem
│   │   ├── base_controller/          # Motor control & odometry
│   │   ├── mobile_base_description/  # URDF/meshes
│   │   └── mobile_base_gazebo/       # Simulation
│   │
│   ├── mobile_manipulator/           # Integrated system
│   │   ├── mobile_manipulator_description/
│   │   ├── mobile_manipulator_hardware/
│   │   ├── mobile_manipulator_navigation/
│   │   └── mobile_manipulator_application/
│   │
│   ├── sensors/                      # Sensor packages
│   │   ├── camera_driver/            # RPi camera + QR detection
│   │   └── lidar_bringup/            # LiDAR launch wrapper
│   │
│   ├── ydlidar_ros2_driver/          # YDLiDAR X2L driver
│   ├── bringup/robot_bringup/        # Master launch files
│   ├── applications/pick_and_place/  # Application logic
│   └── simulation/                   # Gazebo simulation
│
├── arduino/                          # Arduino firmware
├── maps/                             # SLAM saved maps
├── build/                            # Compiled packages
├── install/                          # Installed packages
└── *.md                              # Documentation files
```

---

## Package Summary

### Arm Subsystem (3 packages)

| Package | Purpose | Key Files |
|---------|---------|-----------|
| `arm_description` | URDF model, meshes, kinematics | `arm.urdf.xacro` |
| `arm_controller` | Servo bridge, trajectory execution | `servo_bridge.py`, `trajectory_bridge.py` |
| `arm_moveit` | MoveIt2 motion planning config | `moveit_controllers.yaml`, launch files |

### Mobile Base Subsystem (3 packages)

| Package | Purpose | Key Files |
|---------|---------|-----------|
| `mobile_base_description` | URDF model, wheel geometry | `mobile_base.urdf.xacro` |
| `base_controller` | Motor control, odometry, obstacle avoidance | `base_hardware_bridge.py` |
| `mobile_base_gazebo` | Gazebo simulation plugins | world files |

### Mobile Manipulator (4 packages)

| Package | Purpose | Key Files |
|---------|---------|-----------|
| `mobile_manipulator_description` | Combined URDF (arm + base) | `mobile_manipulator.urdf.xacro` |
| `mobile_manipulator_hardware` | Unified hardware interface | `hardware_params.yaml` |
| `mobile_manipulator_navigation` | Nav2 + SLAM config | `nav2_params.yaml`, `slam_params.yaml` |
| `mobile_manipulator_application` | State machines | Application scripts |

### Sensors (2 packages)

| Package | Purpose | Key Files |
|---------|---------|-----------|
| `camera_driver` | RPi camera, QR code detection | `qr_scanner.py` |
| `lidar_bringup` | YDLiDAR launch wrapper | launch files |

---

## Hardware Configuration

### Components

| Component | Model | Connection | Serial Port |
|-----------|-------|------------|-------------|
| MCU | Arduino Mega 2560 | USB | `/dev/ttyACM0` |
| Servo Driver | PCA9685 | I2C to Arduino | - |
| Motor Drivers | 2x L298N | GPIO to Arduino | - |
| LiDAR | YDLiDAR X2L | USB | `/dev/ttyUSB0` |
| Camera | RPi Camera v2 | CSI | - |

### Servo Channel Mapping

| Channel | Joint | Range |
|---------|-------|-------|
| CH0 | joint_1 (base) | 0-180° |
| CH1 | joint_2 (shoulder) | 0-180° |
| CH2 | joint_3 (elbow) | 0-180° |
| CH3 | joint_4 (wrist1) | 0-180° |
| CH4 | gripper_base_joint | 0-180° |
| CH5 | left_gear_joint (gripper) | 90-135° |

### Serial Protocol

**Commands (PC/RPi → Arduino):**
```
VEL,<linear>,<angular>     # Set base velocity
ARM,<a0>,<a1>,...,<a5>     # Set all servo angles
SERVO,<ch>,<angle>         # Set single servo
STOP                       # Emergency stop
STATUS                     # Request status
```

**Responses (Arduino → PC/RPi):**
```
ENC,<fl>,<fr>,<bl>,<br>    # Encoder ticks
ARM_POS,<a0>,...,<a5>      # Current servo positions
HEARTBEAT,<ms>,OBS:<0|1>   # Periodic heartbeat
OK,<command>               # Command acknowledged
```

---

## Key Launch Files

| Launch File | Purpose | Command |
|-------------|---------|---------|
| `robot.launch.py` | Full hardware bringup | `ros2 launch robot_bringup robot.launch.py` |
| `slam.launch.py` | SLAM mapping mode | `ros2 launch robot_bringup slam.launch.py` |
| `navigation.launch.py` | Autonomous navigation | `ros2 launch robot_bringup navigation.launch.py` |
| `demo.launch.py` | MoveIt arm demo | `ros2 launch arm_moveit demo.launch.py` |
| `pick_and_place.launch.py` | Full application | `ros2 launch robot_bringup pick_and_place.launch.py` |

---

## Dependencies

### ROS2 Packages
- `ros2-humble-navigation2`
- `ros2-humble-slam-toolbox`
- `ros2-humble-moveit`
- `ros2-humble-ros2-control`
- `ros2-humble-gazebo-ros-pkgs`

### Python Libraries
- `pyserial` - Serial communication
- `opencv-python` - Computer vision
- `pyzbar` - QR code detection
- `numpy` - Numerical operations

---

## Quick Start

### 1. Build Workspace
```bash
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
colcon build --symlink-install
source install/setup.bash
```

### 2. Launch Robot Hardware
```bash
ros2 launch robot_bringup robot.launch.py
```

### 3. Test Arm (MoveIt)
```bash
ros2 launch arm_moveit demo.launch.py
```

### 4. SLAM Mapping
```bash
ros2 launch robot_bringup slam.launch.py
```

### 5. Save Map
```bash
ros2 run nav2_map_server map_saver_cli -f ~/map
```

---

## ROS2 Topics

| Topic | Type | Publisher | Subscriber |
|-------|------|-----------|------------|
| `/cmd_vel` | Twist | Nav2/Teleop | base_hardware_bridge |
| `/odom` | Odometry | base_hardware_bridge | Nav2 |
| `/scan` | LaserScan | ydlidar_driver | Nav2/SLAM |
| `/joint_states` | JointState | servo_bridge | MoveIt/RViz |
| `/tf` | TFMessage | robot_state_publisher | All |
| `/arm/joint_commands` | JointState | MoveIt | servo_bridge |
| `/qr_scanner/data` | String | qr_scanner | pick_and_place |

---

## Existing Documentation

| File | Description |
|------|-------------|
| `README.md` | Main project overview |
| `QUICK_START.md` | Setup instructions |
| `CONNECTION_GUIDE.md` | Hardware/network setup |
| `LIDAR_FINDINGS.md` | LiDAR configuration details |
| `ARM_TESTING.md` | Arm servo testing results |
| `ROS2_PLAYBOOK.md` | ROS2 concepts guide |
| `OBSTACLE_AVOIDANCE.md` | Obstacle avoidance setup |

---

## Known Issues & Troubleshooting

1. **Serial Port Permissions**
   ```bash
   sudo usermod -aG dialout $USER
   # Log out and log back in
   ```

2. **LiDAR Not Detected**
   ```bash
   ls -la /dev/ttyUSB*
   sudo chmod 666 /dev/ttyUSB0
   ```

3. **MoveIt Controller Issues**
   - Ensure `trajectory_bridge.py` is running
   - Check joint limits in `joint_limits.yaml`

4. **Odometry Drift**
   - Calibrate wheel radius and encoder CPR
   - Tune in `hardware_params.yaml`

---

## Development Roadmap

### Completed
- [x] 4-wheel differential drive base
- [x] 6-DOF robotic arm with gripper
- [x] LiDAR integration (YDLiDAR X2L)
- [x] SLAM mapping capability
- [x] MoveIt2 motion planning
- [x] Basic obstacle avoidance
- [x] QR code detection

### In Progress
- [ ] Nav2 autonomous navigation tuning
- [ ] Pick-and-place application refinement
- [ ] Web control panel integration

### Future
- [ ] Computer vision object detection
- [ ] Voice command integration
- [ ] Multi-robot coordination
- [ ] ROS2 Control migration

---

*Generated: 2026-01-06*
