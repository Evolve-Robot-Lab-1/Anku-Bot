#!/bin/bash
# Obstacle Avoidance System - RPi Startup Script
# Copy this to RPi: scp start_rpi_base.sh pi@192.168.1.27:~/
# Then run: chmod +x ~/start_rpi_base.sh && ~/start_rpi_base.sh

source /opt/ros/humble/setup.bash
source /home/pi/pi_ros_ws/gripper_car_ws/install/setup.bash

# ROS2 Network Configuration
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

echo "Setting device permissions..."
sudo chmod 666 /dev/ttyUSB0 2>/dev/null
sudo chmod 666 /dev/ttyACM0 2>/dev/null

echo "Starting YDLiDAR driver..."
ros2 run ydlidar_ros2_driver ydlidar_ros2_driver_node \
  --ros-args \
  -p port:=/dev/ttyUSB0 \
  -p baudrate:=115200 \
  -p lidar_type:=1 \
  -p isSingleChannel:=true \
  -p support_motor_dtr:=true &
LIDAR_PID=$!

sleep 3

echo "Starting Base Hardware Bridge..."
ros2 run base_controller base_hardware_bridge.py &
BRIDGE_PID=$!

echo ""
echo "=========================================="
echo "RPi nodes started!"
echo "  LiDAR PID: $LIDAR_PID"
echo "  Bridge PID: $BRIDGE_PID"
echo ""
echo "Now run on Local PC:"
echo "  ./scripts/start_obstacle_avoidance.sh"
echo "=========================================="
echo ""
echo "Press Ctrl+C to stop all nodes"

# Wait and cleanup on exit
trap "kill $LIDAR_PID $BRIDGE_PID 2>/dev/null; exit" INT TERM
wait
