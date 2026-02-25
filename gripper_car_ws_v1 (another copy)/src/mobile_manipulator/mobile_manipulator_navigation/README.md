# mobile_manipulator_navigation

ROS2 package providing Nav2 configuration and SLAM capabilities for the mobile manipulator.

## Table of Contents

- [Overview](#overview)
- [Package Structure](#package-structure)
- [SLAM Mapping](#slam-mapping)
- [Navigation](#navigation)
- [Configuration Files](#configuration-files)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)

---

## Overview

This package provides autonomous navigation capabilities using:

- **SLAM Toolbox**: Create maps of unknown environments
- **Nav2 Stack**: Path planning and autonomous navigation
- **AMCL**: Localization in known maps

### Navigation Stack Components

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Nav2 Stack                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │   Planner   │  │  Controller │  │  Behaviors  │  │   BT Nav    │    │
│  │  (NavFn)    │  │    (DWB)    │  │  (Recovery) │  │ (Behavior   │    │
│  │             │  │             │  │             │  │    Tree)    │    │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘    │
│         │                │                │                │            │
│         └────────────────┴────────────────┴────────────────┘            │
│                                    │                                     │
│                    ┌───────────────┴───────────────┐                     │
│                    │        Costmap 2D             │                     │
│                    │  (global + local costmaps)    │                     │
│                    └───────────────┬───────────────┘                     │
└────────────────────────────────────┼─────────────────────────────────────┘
                                     │
                    ┌────────────────┼────────────────┐
                    ▼                ▼                ▼
               /scan              /odom            /tf
              (LiDAR)           (wheels)         (robot)
```

---

## Package Structure

```
mobile_manipulator_navigation/
├── CMakeLists.txt
├── package.xml
├── README.md
├── config/
│   ├── nav2_params.yaml          # Full Nav2 configuration
│   ├── slam_params.yaml          # SLAM Toolbox configuration
│   ├── mapping_params.yaml       # Mapping mode parameters
│   └── localisation_params.yaml  # AMCL localization
├── launch/
│   ├── navigation.launch.py      # Nav2 with pre-built map
│   └── slam_mapping.launch.py    # SLAM for new maps
└── maps/
    ├── test_arena_map.yaml       # Map metadata
    └── test_arena_map.pgm        # Occupancy grid image
```

---

## SLAM Mapping

Create maps of unknown environments using SLAM Toolbox.

### How SLAM Works

1. Robot moves through environment with teleop
2. LiDAR scans are matched to build a consistent map
3. Pose graph optimization maintains accuracy
4. Map is saved for future navigation

### Launch SLAM Mapping

```bash
# Launch SLAM mapping mode
ros2 launch mobile_manipulator_navigation slam_mapping.launch.py

# In another terminal: control robot with teleop
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

### Saving the Map

```bash
# When mapping is complete, save the map
ros2 run nav2_map_server map_saver_cli -f ~/maps/my_map

# This creates:
#   my_map.yaml - metadata
#   my_map.pgm  - image file
```

### SLAM Parameters (mapping_params.yaml)

```yaml
slam_toolbox:
  ros__parameters:
    # Mode
    mode: mapping

    # Scan matching
    use_scan_matching: true
    max_laser_range: 12.0
    minimum_travel_distance: 0.5
    minimum_travel_heading: 0.5

    # Map update
    map_update_interval: 5.0
    resolution: 0.05

    # Pose graph optimization
    do_loop_closing: true
    loop_search_maximum_distance: 3.0
```

---

## Navigation

Autonomous navigation in known maps using Nav2.

### Navigation Components

| Component | Purpose |
|-----------|---------|
| **Map Server** | Loads and provides the map |
| **AMCL** | Localizes robot in map |
| **Global Planner** | Plans path to goal |
| **Local Controller** | Follows path, avoids obstacles |
| **Recovery Behaviors** | Handles stuck situations |
| **BT Navigator** | Orchestrates navigation |

### Launch Navigation

```bash
# Launch with pre-built map
ros2 launch mobile_manipulator_navigation navigation.launch.py map:=/path/to/map.yaml

# Default map location
ros2 launch mobile_manipulator_navigation navigation.launch.py
```

### Send Navigation Goals

**Using RViz:**
1. Click "2D Goal Pose" button
2. Click and drag on map to set goal

**Using Command Line:**
```bash
ros2 action send_goal /navigate_to_pose nav2_msgs/action/NavigateToPose \
  "{pose: {header: {frame_id: 'map'}, pose: {position: {x: 2.0, y: 1.0}}}}"
```

**Using Python:**
```python
from nav2_simple_commander.robot_navigator import BasicNavigator
from geometry_msgs.msg import PoseStamped

navigator = BasicNavigator()
goal_pose = PoseStamped()
goal_pose.header.frame_id = 'map'
goal_pose.pose.position.x = 2.0
goal_pose.pose.position.y = 1.0
navigator.goToPose(goal_pose)
```

---

## Configuration Files

### nav2_params.yaml

Complete Nav2 stack configuration:

```yaml
# Controller Server (DWB - Dynamic Window Approach)
controller_server:
  ros__parameters:
    controller_frequency: 20.0
    FollowPath:
      plugin: "dwb_core::DWBLocalPlanner"
      max_vel_x: 0.5
      min_vel_x: 0.0
      max_vel_theta: 1.0
      acc_lim_x: 2.5
      acc_lim_theta: 3.2

# Planner Server (NavFn)
planner_server:
  ros__parameters:
    GridBased:
      plugin: "nav2_navfn_planner/NavfnPlanner"
      tolerance: 0.5

# Costmap configuration
global_costmap:
  ros__parameters:
    update_frequency: 1.0
    publish_frequency: 1.0
    robot_radius: 0.25
    plugins: ["static_layer", "obstacle_layer", "inflation_layer"]

local_costmap:
  ros__parameters:
    update_frequency: 5.0
    publish_frequency: 2.0
    robot_radius: 0.25
    plugins: ["obstacle_layer", "inflation_layer"]
```

### Key Parameters to Tune

| Parameter | Description | Tuning Tips |
|-----------|-------------|-------------|
| `robot_radius` | Robot collision footprint | Measure actual robot |
| `max_vel_x` | Maximum forward speed | Start low, increase carefully |
| `inflation_radius` | Safety buffer around obstacles | Higher = more cautious |
| `tolerance` | Goal arrival tolerance | Lower = more precise |

---

## Usage

### Complete Navigation Workflow

#### Step 1: Create a Map

```bash
# Terminal 1: Launch robot hardware
ros2 launch robot_bringup robot.launch.py

# Terminal 2: Launch SLAM
ros2 launch mobile_manipulator_navigation slam_mapping.launch.py

# Terminal 3: Teleop to drive around
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Terminal 4: Save map when done
ros2 run nav2_map_server map_saver_cli -f ~/my_map
```

#### Step 2: Navigate with Map

```bash
# Terminal 1: Launch robot hardware
ros2 launch robot_bringup robot.launch.py

# Terminal 2: Launch navigation
ros2 launch mobile_manipulator_navigation navigation.launch.py \
  map:=~/my_map.yaml

# Terminal 3: Set initial pose in RViz, then send goals
```

### Localization Only

If using external localization (e.g., from SLAM Toolbox in localization mode):

```bash
ros2 run slam_toolbox localization_slam_toolbox_node \
  --ros-args -p map_file_name:=~/my_map
```

---

## Map Files

### Map Format

Maps consist of two files:

**my_map.yaml** (metadata):
```yaml
image: my_map.pgm
resolution: 0.05       # meters per pixel
origin: [-10.0, -10.0, 0.0]  # [x, y, theta]
occupied_thresh: 0.65
free_thresh: 0.196
negate: 0
```

**my_map.pgm** (occupancy grid):
- White (255): Free space
- Black (0): Occupied
- Gray (205): Unknown

### Editing Maps

You can edit maps with image editors:
1. Open PGM file in GIMP/Photoshop
2. Draw obstacles (black) or free space (white)
3. Save as PGM format

---

## Troubleshooting

### Robot Not Localizing

1. Set initial pose in RViz ("2D Pose Estimate")
2. Check AMCL is receiving scans:
```bash
ros2 topic echo /scan --once
```
3. Verify TF is complete:
```bash
ros2 run tf2_tools view_frames
```

### Path Planning Fails

1. Check costmaps in RViz (add Costmap displays)
2. Verify robot_radius is correct
3. Ensure map has clear path to goal
4. Check for TF errors:
```bash
ros2 topic echo /tf_static
```

### Robot Oscillates/Wobbles

1. Reduce max velocity:
```yaml
max_vel_x: 0.3  # Lower value
```
2. Adjust PID gains in controller

### Recovery Behaviors Trigger Often

1. Increase goal tolerance
2. Reduce costmap update frequency
3. Check sensor data quality

### SLAM Map Quality Issues

1. Drive slowly during mapping
2. Revisit areas to close loops
3. Adjust scan matching parameters:
```yaml
minimum_travel_distance: 0.3  # More frequent updates
loop_search_maximum_distance: 5.0  # Larger loop detection
```

---

## Dependencies

- `nav2_bringup` - Nav2 launch files
- `nav2_bt_navigator` - Behavior tree navigator
- `nav2_controller` - Path following
- `nav2_planner` - Path planning
- `nav2_behaviors` - Recovery behaviors
- `nav2_map_server` - Map loading
- `nav2_amcl` - Localization
- `nav2_lifecycle_manager` - Node lifecycle
- `slam_toolbox` - SLAM mapping
- `tf2_ros` - TF transforms

---

## Related Packages

- [mobile_manipulator_description](../mobile_manipulator_description/README.md) - Robot model
- [base_controller](../../mobile_base/base_controller/README.md) - Odometry source
- [lidar_bringup](../../sensors/lidar_bringup/README.md) - LiDAR data
- [pick_and_place](../../applications/pick_and_place/README.md) - Navigation in application
