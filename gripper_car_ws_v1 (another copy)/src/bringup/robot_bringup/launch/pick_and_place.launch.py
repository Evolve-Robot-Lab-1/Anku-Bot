"""
Pick and Place Launch File
==========================
Launches the complete pick-and-place application:
- Robot hardware (arm + base)
- Sensors (LiDAR + camera)
- Navigation (Nav2)
- MoveIt for arm control
- Pick and place application nodes

Usage:
    ros2 launch robot_bringup pick_and_place.launch.py map:=/path/to/map.yaml
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
    """Generate launch description for pick and place."""

    # Get package directories
    nav_pkg = get_package_share_directory('mobile_manipulator_navigation')
    app_pkg = get_package_share_directory('mobile_manipulator_application')

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

    # ===== ROBOT HARDWARE =====
    robot_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('robot_bringup'),
                'launch',
                'robot.launch.py'
            ])
        ]),
        launch_arguments={
            'use_rviz': 'false',
            'use_lidar': 'true',
        }.items()
    )

    # ===== NAVIGATION =====
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

    # ===== MOVEIT =====
    moveit_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('arm_moveit'),
                'launch',
                'move_group.launch.py'
            ])
        ]),
        launch_arguments={
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }.items()
    )

    # ===== CAMERA =====
    camera_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('camera_driver'),
                'launch',
                'camera.launch.py'
            ])
        ])
    )

    # ===== PICK AND PLACE APPLICATION =====
    app_params = os.path.join(app_pkg, 'config', 'pick_and_place_params.yaml')

    pick_and_place_node = Node(
        package='mobile_manipulator_application',
        executable='pick_and_place_node',
        name='pick_and_place',
        output='screen',
        parameters=[app_params],
    )

    # ===== RVIZ =====
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
    )

    return LaunchDescription([
        # Arguments
        map_arg,
        use_sim_time_arg,

        # Launches
        robot_launch,
        nav2_launch,
        moveit_launch,
        camera_launch,
        pick_and_place_node,
        rviz_node,
    ])
