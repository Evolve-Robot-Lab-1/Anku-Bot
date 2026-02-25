#!/usr/bin/env python3
"""
YDLIDAR X2 launch (driver only)

This launch file starts the `ydlidar_ros2_driver` node using the
X2-specific parameter file (`params/X2L.yaml`) bundled with the
`ydlidar_ros2_driver` package. It exposes two launch arguments that
can override the serial port and frame id from the params file.
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    # Launch args
    lidar_port = LaunchConfiguration('lidar_port')
    lidar_frame = LaunchConfiguration('lidar_frame')

    # Locate parameter file in ydlidar driver package
    pkg_share = get_package_share_directory('ydlidar_ros2_driver')
    lidar_params_file = os.path.join(pkg_share, 'params', 'X2L.yaml')

    # ydlidar driver node (using packaged X2 params, with overrides)
    ydlidar_node = Node(
        package='ydlidar_ros2_driver',
        executable='ydlidar_ros2_driver_node',
        name='ydlidar',
        parameters=[lidar_params_file,
                    {'port': lidar_port, 'frame_id': lidar_frame}],
        output='screen'
    )

    return LaunchDescription([
        DeclareLaunchArgument('lidar_port', default_value='/dev/ttyUSB0',
                               description='Serial port for YDLIDAR X2'),
        DeclareLaunchArgument('lidar_frame', default_value='laser_frame',
                               description='TF frame id for the LiDAR scans'),
        ydlidar_node,
    ])
