"""
Hardware Test Launch File
==========================
Launches the mobile manipulator for microcontroller testing.
Uses Arduino TEST mode for simulated encoder feedback.

Usage:
    # Basic test (requires Arduino connected)
    ros2 launch robot_bringup test_hardware.launch.py

    # With RViz visualization
    ros2 launch robot_bringup test_hardware.launch.py use_rviz:=true

    # Custom serial port
    ros2 launch robot_bringup test_hardware.launch.py serial_port:=/dev/ttyUSB0

After launch, enable Arduino test mode:
    ros2 topic pub --once /hardware/command std_msgs/String "data: 'TEST,ON'"

Or use the test script:
    ros2 run robot_bringup test_hardware_commands.py
"""

import os
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    TimerAction,
    LogInfo,
)
from launch.conditions import IfCondition
from launch.substitutions import (
    LaunchConfiguration,
    Command,
)
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description for hardware testing."""

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
        description='Launch RViz for visualization'
    )

    enable_test_mode_arg = DeclareLaunchArgument(
        'enable_test_mode',
        default_value='true',
        description='Enable Arduino TEST mode (simulated encoders)'
    )

    # ===== ROBOT DESCRIPTION =====
    urdf_file = os.path.join(
        mm_desc_pkg, 'urdf', 'mobile_manipulator.urdf.xacro'
    )

    robot_description = ParameterValue(
        Command(['xacro ', urdf_file]),
        value_type=str
    )

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

    # ===== ENABLE TEST MODE =====
    # Wait for hardware node to connect, then enable test mode
    enable_test_mode = TimerAction(
        period=4.0,  # Wait 4 seconds for serial connection
        actions=[
            LogInfo(msg="Enabling Arduino TEST mode..."),
            ExecuteProcess(
                cmd=[
                    'ros2', 'topic', 'pub', '--once',
                    '/hardware/command', 'std_msgs/String',
                    "data: 'TEST,ON'"
                ],
                output='screen',
            ),
        ],
        condition=IfCondition(LaunchConfiguration('enable_test_mode'))
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
        enable_test_mode_arg,

        # Info
        LogInfo(msg="=== Mobile Manipulator Hardware Test ==="),
        LogInfo(msg="Connect Arduino Mega 2560 to serial port"),
        LogInfo(msg="Arduino TEST mode simulates encoder feedback"),

        # Nodes
        robot_state_publisher,
        hardware_node,
        enable_test_mode,
        rviz_node,
    ])
