# Session Notes - 2026-01-03

## Project: ROS2 Mobile Manipulator Workspace (gripper_car_ws_v1)

## Current Focus: LiDAR Integration

---

## Hardware Overview
- **Mobile Base:** 4-wheel differential drive with encoders
- **Arm:** 6-DOF servo arm with gripper (MG996R servos, PCA9685 driver)
- **LiDAR:** YDLiDAR X2L
- **Camera:** Raspberry Pi Camera
- **Controller:** Arduino Mega 2560

## Software Stack
- ROS2 Humble
- MoveIt2 (motion planning)
- Nav2 (navigation)
- SLAM Toolbox
- Gazebo simulation

---

## Previous Session Summary (2026-01-02)

### Issues Fixed:
1. **YDLiDAR SDK dependency** - Copied `lidar.xacro` to `mobile_manipulator_sim/urdf/` to bypass SDK requirement for simulation
2. **Missing ompl_planning.yaml** - Created MoveIt OMPL planning config in `arm_moveit/config/`
3. **YAML format error** - Fixed `request_adapters` parameter (must be space-separated string, not YAML array)

### Outstanding Issue:
- `ros-humble-gazebo-ros2-control` package installation issues
- Needed for arm joint control in Gazebo simulation

### Simulation Status:
- Gazebo launches successfully
- Mobile base (diff_drive) works
- LiDAR plugin works in simulation
- Camera plugin works
- MoveIt launches but arm control not functional without gazebo_ros2_control

---

## YDLiDAR X2L Configuration

| Parameter | Value |
|-----------|-------|
| Model | X2L (Model 6) |
| Baudrate | 115200 |
| Sample Rate | 3K |
| Range | 0.10 - 8.0m |
| Frequency | 4-8 Hz (PWM) |
| SingleChannel | true |
| DTR Motor Support | true |
| Intensity | false |
| Reversion | false |

### Config File Location:
`src/ydlidar_ros2_driver/params/X2L.yaml`

### Launch Command:
```bash
ros2 launch ydlidar_ros2_driver ydlidar_launch.py \
  params_file:=$(ros2 pkg prefix ydlidar_ros2_driver)/share/ydlidar_ros2_driver/params/X2L.yaml
```

---

## Next Steps

### For LiDAR:
1. [ ] Install YDLiDAR SDK if not installed
2. [ ] Test hardware connection (`ls /dev/ttyUSB*`)
3. [ ] Run LiDAR driver and verify `/scan` topic
4. [ ] Visualize in RViz
5. [ ] Integrate with SLAM/Nav2

### For Simulation:
1. [ ] Install `ros-humble-gazebo-ros2-control`
2. [ ] Test arm control with MoveIt in simulation

### For Hardware:
1. [ ] Test Arduino communication
2. [ ] Test mobile base movement
3. [ ] Test arm servo control

---

## Key Commands

```bash
# Build workspace
cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash
colcon build --symlink-install

# Launch simulation
ros2 launch mobile_manipulator_sim sim.launch.py

# Launch LiDAR (hardware)
ros2 launch lidar_bringup lidar.launch.py

# Check scan topic
ros2 topic echo /scan --once
ros2 topic hz /scan

# Visualize
rviz2
```

---

## File Locations

| Component | Path |
|-----------|------|
| Main README | `README.md` |
| LiDAR Driver | `src/ydlidar_ros2_driver/` |
| LiDAR Bringup | `src/sensors/lidar_bringup/` |
| Simulation | `src/simulation/mobile_manipulator_sim/` |
| Arm MoveIt | `src/arm/arm_moveit/` |
| Navigation | `src/mobile_manipulator/mobile_manipulator_navigation/` |

---

## Session End: 2026-01-03
