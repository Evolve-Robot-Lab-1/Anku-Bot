#!/usr/bin/env python3

"""
Complete Arduino Real-Time Demo Launch File
This launch file combines Arduino communication with real-time MoveIt2 controllers
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction, LogInfo
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, TextSubstitution
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Declare launch arguments
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation (Gazebo) clock if true'
    )
    
    use_fake_hardware_arg = DeclareLaunchArgument(
        'use_fake_hardware',
        default_value='false',
        description='Use fake hardware instead of Arduino'
    )
    
    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyACM0',
        description='Serial port for Arduino communication'
    )
    
    baud_rate_arg = DeclareLaunchArgument(
        'baud_rate',
        default_value='115200',
        description='Serial baud rate for Arduino communication'
    )
    
    launch_rviz_arg = DeclareLaunchArgument(
        'launch_rviz',
        default_value='true',
        description='Launch RViz2 with MoveIt configuration'
    )
    
    launch_arduino_bridge_arg = DeclareLaunchArgument(
        'launch_arduino_bridge',
        default_value='true',
        description='Launch Arduino ROS2 bridge'
    )
    
    # Get launch configurations
    use_sim_time = LaunchConfiguration('use_sim_time')
    use_fake_hardware = LaunchConfiguration('use_fake_hardware')
    serial_port = LaunchConfiguration('serial_port')
    baud_rate = LaunchConfiguration('baud_rate')
    launch_rviz = LaunchConfiguration('launch_rviz')
    launch_arduino_bridge = LaunchConfiguration('launch_arduino_bridge')
    
    # Information message
    info_message = LogInfo(
        msg=[
            'Starting Arduino Real-Time Demo:',
            '  Use Sim Time: ', use_sim_time,
            '  Use Fake Hardware: ', use_fake_hardware,
            '  Arduino Serial Port: ', serial_port,
            '  Launch RViz: ', launch_rviz
        ]
    )
    
    # Arduino Bridge (if not using fake hardware)
    arduino_bridge_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare("mobile_arm_manipulator_config"),
                "launch",
                "arduino_bridge.launch.py",
            ])
        ),
        launch_arguments={
            'serial_port': serial_port,
            'baud_rate': baud_rate,
        }.items(),
        condition=IfCondition(launch_arduino_bridge)
    )
    
    # Real-time controllers launch
    realtime_controllers_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare("mobile_arm_manipulator_config"),
                "launch",
                "realtime_controllers.launch.py",
            ])
        ),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'use_fake_hardware': use_fake_hardware,
        }.items(),
    )
    
    # MoveIt move_group launch (delayed to allow controllers to start)
    move_group_launch = TimerAction(
        period=3.0,  # 3 second delay
        actions=[
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    PathJoinSubstitution([
                        FindPackageShare("mobile_arm_manipulator_config"),
                        "launch",
                        "move_group.launch.py",
                    ])
                ),
                launch_arguments={
                    'use_sim_time': use_sim_time,
                }.items(),
            )
        ]
    )
    
    # RViz launch (delayed to allow everything to start)
    rviz_launch = TimerAction(
        period=5.0,  # 5 second delay
        actions=[
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    PathJoinSubstitution([
                        FindPackageShare("mobile_arm_manipulator_config"),
                        "launch",
                        "moveit_rviz.launch.py",
                    ])
                ),
                launch_arguments={
                    'use_sim_time': use_sim_time,
                }.items(),
            )
        ],
        condition=IfCondition(launch_rviz)
    )
    
    return LaunchDescription([
        use_sim_time_arg,
        use_fake_hardware_arg,
        serial_port_arg,
        baud_rate_arg,
        launch_rviz_arg,
        launch_arduino_bridge_arg,
        info_message,
        arduino_bridge_launch,
        realtime_controllers_launch,
        move_group_launch,
        rviz_launch,
    ])
