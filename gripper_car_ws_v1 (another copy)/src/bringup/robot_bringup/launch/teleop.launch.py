"""
Teleop Launch File
==================
Launches robot hardware + teleop keyboard control.

Usage:
    ros2 launch robot_bringup teleop.launch.py
"""

import os
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    ExecuteProcess,
)
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description for teleop mode."""

    # Include robot launch
    robot_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('robot_bringup'),
                'launch',
                'robot.launch.py'
            ])
        ]),
        launch_arguments={
            'use_rviz': 'true',
            'use_lidar': 'true',
        }.items()
    )

    # Teleop keyboard node
    # Uses the existing teleop_keyboard from base_controller package
    teleop_node = Node(
        package='base_controller',
        executable='teleop_keyboard.py',
        name='teleop_keyboard',
        output='screen',
        prefix='xterm -e',  # Launch in separate terminal
        parameters=[{
            'linear_speed': 0.3,
            'angular_speed': 0.5,
        }]
    )

    return LaunchDescription([
        robot_launch,
        teleop_node,
    ])
