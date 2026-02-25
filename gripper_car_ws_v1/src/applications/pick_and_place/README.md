# pick_and_place

ROS2 package implementing a complete pick-and-place application with QR code-based destination routing.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [State Machine](#state-machine)
- [Nodes](#nodes)
- [Configuration](#configuration)
- [Usage](#usage)
- [Integration](#integration)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package provides a complete pick-and-place workflow:

1. Robot navigates to pickup location
2. Arm picks up object
3. QR code on object is scanned
4. Robot navigates to destination (based on QR)
5. Arm places object
6. Robot returns for next object

### Key Features

- **State machine architecture** for robust operation
- **QR code routing** - destinations encoded on objects
- **Error recovery** - automatic retries and fallbacks
- **Continuous mode** - runs indefinitely
- **Configurable timeouts** and retry limits

---

## Package Structure

```
pick_and_place/
├── CMakeLists.txt
├── package.xml
├── README.md
├── scripts/
│   ├── pick_and_place_node.py    # Main state machine
│   ├── arm_interface.py          # MoveIt arm wrapper
│   ├── nav_interface.py          # Nav2 navigation wrapper
│   ├── location_manager.py       # Location database
│   └── qr_scanner_node.py        # QR detection service
├── launch/
│   ├── pick_and_place.launch.py  # Application launcher
│   └── qr_scanner.launch.py      # Scanner only
└── config/
    ├── pick_and_place.yaml       # System configuration
    ├── arm_poses.yaml            # Predefined arm poses
    └── locations.yaml            # Pickup/dropoff locations
```

---

## State Machine

### States

```
        ┌────────────────────────────────────────────────────────────┐
        │                                                            │
        ▼                                                            │
┌──────────────┐     ┌──────────────┐     ┌──────────────┐          │
│     IDLE     │────▶│  GO_PICKUP   │────▶│  PICK_READY  │          │
└──────────────┘     └──────────────┘     └──────────────┘          │
                                                  │                  │
                                                  ▼                  │
                     ┌──────────────┐     ┌──────────────┐          │
                     │   SCANNING   │◀────│   PICKING    │          │
                     └──────────────┘     └──────────────┘          │
                            │                                        │
                            ▼                                        │
                     ┌──────────────┐     ┌──────────────┐          │
                     │  SCAN_POSE   │────▶│  GO_DROPOFF  │          │
                     └──────────────┘     └──────────────┘          │
                                                  │                  │
                                                  ▼                  │
                     ┌──────────────┐     ┌──────────────┐          │
                     │    RETURN    │◀────│   PLACING    │──────────┘
                     └──────────────┘     └──────────────┘
                            │
                            │ (continuous_mode)
                            └─────────────▶ GO_PICKUP
```

### State Descriptions

| State | Description |
|-------|-------------|
| `IDLE` | Waiting for start command |
| `GO_PICKUP` | Navigating to pickup location |
| `PICK_READY` | Moving arm to approach position |
| `PICKING` | Executing pick sequence (approach → grasp → lift) |
| `SCAN_POSE` | Moving arm to show QR to camera |
| `SCANNING` | Reading QR code to determine destination |
| `GO_DROPOFF` | Navigating to destination |
| `PLACING` | Executing place sequence |
| `RETURN` | Returning to pickup for next cycle |
| `ERROR` | Error state, requires reset |
| `PAUSED` | Temporarily paused |

---

## Nodes

### pick_and_place_node.py

Main orchestrator implementing the state machine.

#### Topics

**Subscribed:**

| Topic | Type | Description |
|-------|------|-------------|
| `/pick_and_place/command` | `String` | Control commands |

**Published:**

| Topic | Type | Description |
|-------|------|-------------|
| `/pick_and_place/state` | `String` | Current state name |
| `/pick_and_place/status` | `String` | Detailed status |

#### Commands

| Command | Effect |
|---------|--------|
| `start` | Begin pick-and-place cycle |
| `stop` | Stop and return to IDLE |
| `pause` | Pause current operation |
| `resume` | Resume from pause |
| `reset` | Reset to IDLE, clear errors |

---

### arm_interface.py

Wrapper for MoveIt2 arm control.

#### Functions

```python
class ArmInterface:
    def move_to_config_pose(self, pose_name: str) -> bool
    def move_to_named_pose(self, pose_name: str) -> bool
    def open_gripper(self) -> bool
    def close_gripper(self) -> bool
    def stow(self) -> bool
    def wait_for_action_servers(self, timeout: float) -> bool
```

#### Configured Poses (from arm_poses.yaml)

| Pose | Description |
|------|-------------|
| `approach_pose` | Above pick position |
| `pick_pose` | Grasp position |
| `lift_pose` | Lifted after grasp |
| `scan_pose` | Show QR to camera |
| `place_approach_pose` | Above place position |
| `place_pose` | Release position |
| `stowed` | Safe travel position |

---

### nav_interface.py

Wrapper for Nav2 navigation.

#### Functions

```python
class NavInterface:
    def navigate_to_location(self, location: LocationCoordinates, timeout: float) -> bool
    def cancel_navigation(self) -> None
    def is_navigating(self) -> bool
    def wait_for_nav2(self, timeout: float) -> bool
```

---

### location_manager.py

Manages pickup and dropoff locations.

#### Functions

```python
class LocationManager:
    def get_pickup_location(self) -> LocationCoordinates
    def get_default_location(self) -> LocationCoordinates
    def get_location(self, name: str) -> Optional[LocationCoordinates]
    def list_locations(self) -> List[str]
```

---

## Configuration

### pick_and_place.yaml

```yaml
pick_and_place:
  # Operation mode
  continuous_mode: true   # Keep running after each cycle

  # Timeouts (seconds)
  timeouts:
    navigation: 120.0     # Max time to reach goal
    arm_motion: 30.0      # Max time for arm move
    qr_scan: 10.0         # Max time for QR scan
    gripper_action: 5.0   # Max time for gripper

  # Retry counts
  retries:
    navigation: 3         # Nav failures before error
    arm_motion: 2         # Arm failures before error
    pick_attempt: 2       # Pick failures before error
    qr_scan: 3            # Scan failures before fallback

  # Behavior
  behavior:
    use_default_on_unknown_qr: true   # Use default if QR unknown
    return_on_scan_failure: true      # Return object if can't scan
    state_transition_delay: 0.5       # Delay between states
    gripper_delay: 0.3                # Delay for gripper actions
```

### locations.yaml

```yaml
locations:
  pickup:
    name: "pickup_station"
    pose:
      x: 0.0
      y: 0.0
      theta: 0.0

  dropoff_a:
    name: "Station A"
    pose:
      x: 2.0
      y: 1.0
      theta: 1.57

  dropoff_b:
    name: "Station B"
    pose:
      x: 2.0
      y: -1.0
      theta: -1.57

  default:
    name: "Default Station"
    pose:
      x: 1.0
      y: 0.0
      theta: 0.0

# QR code to location mapping
qr_mapping:
  "STATION_A": "dropoff_a"
  "STATION_B": "dropoff_b"
  "RETURN": "pickup"
```

### arm_poses.yaml

```yaml
arm_poses:
  approach_pose:
    joint_1: 0.0
    joint_2: -0.3
    joint_3: 0.6
    joint_4: 0.0
    gripper_base_joint: 0.0

  pick_pose:
    joint_1: 0.0
    joint_2: 0.2
    joint_3: 0.8
    joint_4: 0.0
    gripper_base_joint: 0.0

  # ... more poses
```

---

## Usage

### Launch Application

```bash
# First: ensure robot is running
ros2 launch robot_bringup robot.launch.py

# Launch navigation
ros2 launch mobile_manipulator_navigation navigation.launch.py \
  map:=/path/to/map.yaml

# Launch MoveIt
ros2 launch arm_moveit move_group.launch.py

# Launch pick and place
ros2 launch pick_and_place pick_and_place.launch.py
```

### Control Commands

```bash
# Start operation
ros2 topic pub /pick_and_place/command std_msgs/String "data: 'start'" --once

# Monitor state
ros2 topic echo /pick_and_place/state

# Monitor status
ros2 topic echo /pick_and_place/status

# Pause operation
ros2 topic pub /pick_and_place/command std_msgs/String "data: 'pause'" --once

# Resume
ros2 topic pub /pick_and_place/command std_msgs/String "data: 'resume'" --once

# Stop
ros2 topic pub /pick_and_place/command std_msgs/String "data: 'stop'" --once

# Reset after error
ros2 topic pub /pick_and_place/command std_msgs/String "data: 'reset'" --once
```

### Using robot_bringup

```bash
# Complete system in one command
ros2 launch robot_bringup pick_and_place.launch.py map:=/path/to/map.yaml
```

---

## Integration

### QR Code Format

Objects should have QR codes containing location names that match `qr_mapping` in `locations.yaml`:

```
QR Content: "STATION_A"  →  Routes to dropoff_a
QR Content: "STATION_B"  →  Routes to dropoff_b
QR Content: "RETURN"     →  Routes back to pickup
```

### Creating QR Codes

```bash
# Install qrencode
sudo apt install qrencode

# Generate QR codes
qrencode -o station_a.png "STATION_A"
qrencode -o station_b.png "STATION_B"
```

### Adding New Locations

1. Edit `locations.yaml`:
```yaml
locations:
  new_station:
    name: "New Station"
    pose:
      x: 3.0
      y: 2.0
      theta: 0.0

qr_mapping:
  "NEW_STATION": "new_station"
```

2. Create QR code for new station

3. Restart pick_and_place node

---

## Troubleshooting

### State Machine Stuck

1. Check current state:
```bash
ros2 topic echo /pick_and_place/state
```

2. Check status for details:
```bash
ros2 topic echo /pick_and_place/status
```

3. Send reset command:
```bash
ros2 topic pub /pick_and_place/command std_msgs/String "data: 'reset'" --once
```

### Navigation Failures

1. Verify Nav2 is running:
```bash
ros2 node list | grep nav
```

2. Check costmaps in RViz

3. Increase navigation timeout in config

### Arm Motion Failures

1. Check MoveIt is running:
```bash
ros2 node list | grep move_group
```

2. Test arm manually in RViz

3. Verify joint limits in arm_poses.yaml

### QR Not Detected

1. Check camera topic:
```bash
ros2 topic hz /camera/image_raw
```

2. Verify lighting conditions

3. Check scan_pose positions arm correctly

4. View detection image:
```bash
ros2 run rqt_image_view rqt_image_view /qr_scanner/detection
```

### Unknown QR Codes

If QR content doesn't match any mapping:
- With `use_default_on_unknown_qr: true` → uses default location
- With `use_default_on_unknown_qr: false` → triggers error

Add new mappings to `locations.yaml` as needed.

---

## Dependencies

- `rclpy` - ROS2 Python client
- `std_msgs` - String messages
- `std_srvs` - Trigger service
- `geometry_msgs` - Pose messages
- `nav2_simple_commander` - Navigation API
- `moveit_py` - MoveIt Python API
- `pyzbar` - QR code detection
- `opencv-python` - Image processing

---

## Related Packages

- [camera_driver](../../sensors/camera_driver/README.md) - Camera and QR scanner
- [arm_moveit](../../arm/arm_moveit/README.md) - Arm motion planning
- [mobile_manipulator_navigation](../../mobile_manipulator/mobile_manipulator_navigation/README.md) - Navigation
- [robot_bringup](../../bringup/robot_bringup/README.md) - System integration
