#!/usr/bin/env python3
"""
ROBOT-SIDE LAUNCH FILE (Raspberry Pi)
======================================
Launches all low-level hardware drivers and state publishers.
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import xacro


def generate_launch_description():
    
    # Package directories
    mobile_manipulator_description_dir = get_package_share_directory('mobile_manipulator_description')
    base_controller_dir = get_package_share_directory('base_controller')
    arm_controller_dir = get_package_share_directory('arm_controller')
    ydlidar_dir = get_package_share_directory('ydlidar_ros2_driver')
    
    # File paths
    xacro_file = os.path.join(mobile_manipulator_description_dir, 'urdf', 'mobile_manipulator.urdf.xacro')
    ydlidar_params_file = os.path.join(ydlidar_dir, 'params', 'X2L.yaml')
    
    # Process xacro
    robot_description_content = xacro.process_file(xacro_file).toxml()
    
    # Launch arguments
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    serial_port = LaunchConfiguration('serial_port', default='/dev/ttyACM0')
    
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation time')
    
    declare_serial_port_cmd = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyACM0',
        description='Serial port for mobile base controller')
    
    # ========================================
    # JOINT STATE AGGREGATOR (CRITICAL!)
    # ========================================
    
    joint_state_aggregator_node = Node(
        package='mobile_manipulator_description',
        executable='joint_state_aggregator.py',
        name='joint_state_aggregator',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )
    
    # ========================================
    # ROBOT STATE PUBLISHER
    # ========================================
    
    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description_content,
            'use_sim_time': use_sim_time
        }]
    )
    
    # NOTE: NO joint_state_publisher here - aggregator handles it
    
    # ========================================
    # HARDWARE DRIVERS
    # ========================================
    
    # Mobile Base Controller (publishes to /base/joint_states)
    base_hardware_controller_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(base_controller_dir, 'launch', 'base_control.launch.py')
        ),
        launch_arguments={
            'serial_port': serial_port,
            'use_rviz': 'false',
            'joint_state_topic': '/base/joint_states'  # ← IMPORTANT
        }.items()
    )
    
    # Arm Controller (publishes to /arm/joint_states)
    arm_hardware_controller_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(arm_controller_dir, 'launch', 'arm_control.launch.py')
        ),
        launch_arguments={
            'use_trajectory_bridge': 'true',
            'joint_state_topic': '/arm/joint_states'  # ← IMPORTANT
        }.items()
    )
    
    # LiDAR Driver
    ydlidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ydlidar_dir, 'launch', 'ydlidar_launch.py')
        ),
        launch_arguments={
            'params_file': ydlidar_params_file
        }.items()
    )
    
    
    # Create launch description
    ld = LaunchDescription()
    
    # Add arguments
    ld.add_action(declare_use_sim_time_cmd)
    ld.add_action(declare_serial_port_cmd)
    
    # IMPORTANT ORDER:
    # 1. Joint state aggregator (creates /joint_states)
    # 2. Robot state publisher (consumes /joint_states)
    # 3. Hardware controllers (publish to /base and /arm topics)

    ld.add_action(base_hardware_controller_launch)
    ld.add_action(arm_hardware_controller_launch)
    ld.add_action(joint_state_aggregator_node)
    ld.add_action(robot_state_publisher_node)

    #ld.add_action(ydlidar_launch)
    
    return ld
