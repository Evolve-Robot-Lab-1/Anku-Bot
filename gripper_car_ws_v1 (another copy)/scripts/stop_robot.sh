#!/bin/bash
# ============================================
# STOP ALL ROBOT NODES (Run on RPi)
# ============================================

echo "Stopping all robot nodes..."

pkill -f serial_bridge
pkill -f ydlidar_ros2_driver
pkill -f camera_node
pkill -f obstacle_avoidance
pkill -f rosbridge
pkill -f web_video_server
pkill -f web_server
pkill -f web_panel

echo "All nodes stopped."
