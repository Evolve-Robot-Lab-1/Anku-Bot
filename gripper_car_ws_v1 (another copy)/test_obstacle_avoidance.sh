#!/bin/bash
# Obstacle Avoidance System - Diagnostic and Test Script
# Date: 2026-01-07

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
RPI_IP="192.168.1.7"
RPI_USER="pi"
WORKSPACE_LOCAL="/media/bhuvanesh/181EAFAA1EAF7F7E/Anku_Bot/ROS/gripper_car_ws_v1"

echo -e "${BLUE}╔══════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Obstacle Avoidance System - Diagnostic Script      ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════╝${NC}"
echo ""

# Test 1: Network Connectivity
echo -e "${YELLOW}[1/8] Testing network connectivity to RPi...${NC}"
if ping -c 2 -W 3 $RPI_IP &> /dev/null; then
    echo -e "${GREEN}✓ RPi reachable at $RPI_IP${NC}"
else
    echo -e "${RED}✗ RPi NOT reachable at $RPI_IP${NC}"
    echo -e "${RED}  Please check:${NC}"
    echo -e "${RED}  - RPi is powered on${NC}"
    echo -e "${RED}  - Network cable connected${NC}"
    echo -e "${RED}  - Both machines on same network${NC}"
    echo ""
    echo -e "${YELLOW}Searching for RPi on network...${NC}"
    sudo nmap -sn 192.168.1.0/24 | grep -B2 "Raspberry" || echo "No Raspberry Pi found"
    exit 1
fi

# Test 2: SSH Connection
echo -e "${YELLOW}[2/8] Testing SSH connection...${NC}"
if ssh -o ConnectTimeout=5 -o BatchMode=yes ${RPI_USER}@${RPI_IP} "echo 'SSH OK'" &> /dev/null; then
    echo -e "${GREEN}✓ SSH connection successful${NC}"
else
    echo -e "${RED}✗ SSH connection failed${NC}"
    echo -e "${RED}  Try: ssh ${RPI_USER}@${RPI_IP}${NC}"
    exit 1
fi

# Test 3: Hardware Detection
echo -e "${YELLOW}[3/8] Checking hardware on RPi...${NC}"
DEVICES=$(ssh ${RPI_USER}@${RPI_IP} "ls /dev/tty* 2>/dev/null | grep -E '(USB|ACM)'" || echo "")

if echo "$DEVICES" | grep -q "ttyUSB"; then
    echo -e "${GREEN}✓ LiDAR found: $(echo "$DEVICES" | grep USB)${NC}"
else
    echo -e "${RED}✗ LiDAR NOT found (expected /dev/ttyUSB0)${NC}"
fi

if echo "$DEVICES" | grep -q "ttyACM"; then
    echo -e "${GREEN}✓ Arduino found: $(echo "$DEVICES" | grep ACM)${NC}"
else
    echo -e "${RED}✗ Arduino NOT found (expected /dev/ttyACM0)${NC}"
fi

# Test 4: ROS2 Installation
echo -e "${YELLOW}[4/8] Checking ROS2 installation...${NC}"
if source /opt/ros/humble/setup.bash 2>/dev/null; then
    ROS_VERSION=$(ros2 --version 2>/dev/null | head -1)
    echo -e "${GREEN}✓ ROS2 installed: $ROS_VERSION${NC}"
else
    echo -e "${RED}✗ ROS2 not found${NC}"
    exit 1
fi

# Test 5: Local Workspace
echo -e "${YELLOW}[5/8] Checking local workspace...${NC}"
if [ -f "$WORKSPACE_LOCAL/install/setup.bash" ]; then
    echo -e "${GREEN}✓ Workspace found and built${NC}"
else
    echo -e "${RED}✗ Workspace not built${NC}"
    echo -e "${YELLOW}  Building now...${NC}"
    cd $WORKSPACE_LOCAL
    source /opt/ros/humble/setup.bash
    colcon build --packages-select base_controller --symlink-install
fi

# Test 6: Check if nodes are already running
echo -e "${YELLOW}[6/8] Checking for running nodes...${NC}"
source /opt/ros/humble/setup.bash
source $WORKSPACE_LOCAL/install/setup.bash

RUNNING_NODES=$(ros2 node list 2>/dev/null || echo "")
if [ -n "$RUNNING_NODES" ]; then
    echo -e "${GREEN}✓ Found running nodes:${NC}"
    echo "$RUNNING_NODES" | sed 's/^/  /'
else
    echo -e "${YELLOW}⚠ No nodes currently running${NC}"
fi

# Test 7: Check topics
echo -e "${YELLOW}[7/8] Checking ROS topics...${NC}"
TOPICS=$(ros2 topic list 2>/dev/null || echo "")
if echo "$TOPICS" | grep -q "/scan"; then
    echo -e "${GREEN}✓ /scan topic available (LiDAR running)${NC}"
else
    echo -e "${YELLOW}⚠ /scan topic not found (LiDAR not running)${NC}"
fi

if echo "$TOPICS" | grep -q "/obstacle_detected"; then
    echo -e "${GREEN}✓ /obstacle_detected topic available (NEW!)${NC}"
else
    echo -e "${YELLOW}⚠ /obstacle_detected topic not found${NC}"
fi

# Test 8: Arduino Communication
echo -e "${YELLOW}[8/8] Testing Arduino communication...${NC}"
ARDUINO_RESPONSE=$(ssh ${RPI_USER}@${RPI_IP} "timeout 2 sh -c 'echo STATUS > /dev/ttyACM0 && cat /dev/ttyACM0' 2>/dev/null | head -1" || echo "")

if [ -n "$ARDUINO_RESPONSE" ]; then
    echo -e "${GREEN}✓ Arduino responding: $ARDUINO_RESPONSE${NC}"
else
    echo -e "${YELLOW}⚠ Arduino not responding (may need reset)${NC}"
fi

# Summary
echo ""
echo -e "${BLUE}╔══════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Diagnostic Summary                                  ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════╝${NC}"
echo ""

if [ -n "$RUNNING_NODES" ] && echo "$TOPICS" | grep -q "/obstacle_detected"; then
    echo -e "${GREEN}✓ System is READY TO TEST!${NC}"
    echo ""
    echo -e "${YELLOW}Next steps:${NC}"
    echo "1. If nodes not running, start them (see OBSTACLE_AVOIDANCE_FIXED.md)"
    echo "2. Run teleop: ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r cmd_vel:=/cmd_vel_raw"
    echo "3. Place obstacle and test avoidance"
else
    echo -e "${YELLOW}⚠ System needs setup${NC}"
    echo ""
    echo -e "${YELLOW}Follow the Quick Start Guide in:${NC}"
    echo "  OBSTACLE_AVOIDANCE_FIXED.md"
fi

echo ""
echo -e "${BLUE}To monitor system in real-time:${NC}"
echo "  ros2 topic echo /obstacle_avoidance/status"
echo "  ros2 topic echo /obstacle_detected"
echo "  ros2 topic hz /scan"
echo ""
