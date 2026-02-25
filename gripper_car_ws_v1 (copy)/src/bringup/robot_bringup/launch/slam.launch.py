"""
SLAM Mapping Launch File
========================
Launches robot hardware + SLAM Toolbox for mapping.

Usage:
    ros2 launch robot_bringup slam.launch.py

To save map:
    ros2 run nav2_map_server map_saver_cli -f ~/maps/my_map
"""

import os
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
)
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description for SLAM mapping."""

    # Get package directories
    nav_pkg = get_package_share_directory('mobile_manipulator_navigation')

    # Arguments
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation time'
    )

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

    # SLAM Toolbox - online async mode for real-time mapping
    slam_params_file = os.path.join(nav_pkg, 'config', 'slam_params.yaml')

    slam_node = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            slam_params_file,
            {'use_sim_time': LaunchConfiguration('use_sim_time')},
        ],
    )

    # Teleop for manual control during mapping
    teleop_node = Node(
        package='base_controller',
        executable='teleop_keyboard.py',
        name='teleop_keyboard',
        output='screen',
        prefix='xterm -e',
        parameters=[{
            'linear_speed': 0.2,  # Slower for mapping
            'angular_speed': 0.4,
        }]
    )

    return LaunchDescription([
        use_sim_time_arg,
        robot_launch,
        slam_node,
        teleop_node,
    ])
