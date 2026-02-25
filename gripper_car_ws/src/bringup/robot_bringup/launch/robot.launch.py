"""
Robot Bringup Launch File
==========================
Launches the complete mobile manipulator hardware stack:
- Robot State Publisher (URDF/TF)
- Hardware Interface (Arduino bridge)
- LiDAR driver

Usage:
    ros2 launch robot_bringup robot.launch.py
    ros2 launch robot_bringup robot.launch.py serial_port:=/dev/ttyUSB0
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.substitutions import (
    LaunchConfiguration,
    Command,
    PathJoinSubstitution,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description for robot hardware."""

    # Package directories
    mm_desc_pkg = get_package_share_directory('mobile_manipulator_description')
    mm_hw_pkg = get_package_share_directory('mobile_manipulator_hardware')

    # ===== LAUNCH ARGUMENTS =====
    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyACM0',
        description='Arduino serial port'
    )

    use_rviz_arg = DeclareLaunchArgument(
        'use_rviz',
        default_value='false',
        description='Launch RViz'
    )

    use_lidar_arg = DeclareLaunchArgument(
        'use_lidar',
        default_value='true',
        description='Launch LiDAR driver'
    )

    # ===== ROBOT DESCRIPTION =====
    urdf_file = os.path.join(
        mm_desc_pkg, 'urdf', 'mobile_manipulator.urdf.xacro'
    )

    robot_description = Command(['xacro ', urdf_file])

    # Robot State Publisher
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description,
            'publish_frequency': 50.0,
        }]
    )

    # ===== HARDWARE INTERFACE =====
    hardware_node = Node(
        package='mobile_manipulator_hardware',
        executable='hardware_node',
        name='hardware_node',
        output='screen',
        parameters=[
            os.path.join(mm_hw_pkg, 'config', 'hardware_params.yaml'),
            {'serial_port': LaunchConfiguration('serial_port')},
        ]
    )

    # ===== LIDAR =====
    lidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('ydlidar_ros2_driver'),
                'launch',
                'ydlidar_launch.py'
            ])
        ]),
        condition=IfCondition(LaunchConfiguration('use_lidar'))
    )

    # ===== RVIZ =====
    rviz_config = os.path.join(mm_desc_pkg, 'config', 'display.rviz')
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config],
        output='screen',
        condition=IfCondition(LaunchConfiguration('use_rviz'))
    )

    return LaunchDescription([
        # Arguments
        serial_port_arg,
        use_rviz_arg,
        use_lidar_arg,

        # Nodes
        robot_state_publisher,
        hardware_node,
        lidar_launch,
        rviz_node,
    ])
