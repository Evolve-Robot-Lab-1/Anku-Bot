"""
Navigation Launch File
======================
Launches robot hardware + Nav2 for autonomous navigation.
Requires a pre-built map.

Usage:
    ros2 launch robot_bringup navigation.launch.py map:=/path/to/map.yaml
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
    """Generate launch description for navigation."""

    # Get package directories
    nav_pkg = get_package_share_directory('mobile_manipulator_navigation')
    nav2_bringup_pkg = get_package_share_directory('nav2_bringup')

    # Arguments
    map_arg = DeclareLaunchArgument(
        'map',
        description='Full path to map yaml file'
    )

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
            'use_rviz': 'false',  # Use Nav2's RViz config
            'use_lidar': 'true',
        }.items()
    )

    # Nav2 bringup
    nav2_params = os.path.join(nav_pkg, 'config', 'nav2_params.yaml')

    nav2_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('nav2_bringup'),
                'launch',
                'bringup_launch.py'
            ])
        ]),
        launch_arguments={
            'map': LaunchConfiguration('map'),
            'use_sim_time': LaunchConfiguration('use_sim_time'),
            'params_file': nav2_params,
        }.items()
    )

    # RViz with Nav2 config
    rviz_config = os.path.join(
        nav2_bringup_pkg, 'rviz', 'nav2_default_view.rviz'
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config],
        output='screen',
    )

    return LaunchDescription([
        map_arg,
        use_sim_time_arg,
        robot_launch,
        nav2_launch,
        rviz_node,
    ])
