#!/bin/bash
# Obstacle Avoidance System - Local PC Startup Script
# Run this on your local PC after starting the RPi nodes

cd /media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1
source /opt/ros/humble/setup.bash
source install/setup.bash

# ROS2 Network Configuration (must match RPi)
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

echo "Starting Obstacle Avoidance Node..."
ros2 run base_controller obstacle_avoidance_node.py &
OBS_PID=$!

sleep 2

echo "Starting Teleop Keyboard..."
echo "Controls: i=forward, k=stop, j=left, l=right"
echo "Press Ctrl+C to exit"
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r cmd_vel:=/cmd_vel_raw

# Cleanup on exit
kill $OBS_PID 2>/dev/null
