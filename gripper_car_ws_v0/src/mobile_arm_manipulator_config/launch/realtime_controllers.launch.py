#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory
import xacro

def generate_launch_description():
    # Declare launch arguments
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation (Gazebo) clock if true'
    )
    
    use_fake_hardware_arg = DeclareLaunchArgument(
        'use_fake_hardware',
        default_value='true',
        description='Start robot with fake hardware mirror of command values to its states'
    )
    
    fake_sensor_commands_arg = DeclareLaunchArgument(
        'fake_sensor_commands',
        default_value='false',
        description='Enable fake command interfaces for sensors used for simple simulations. Used only if \'use_fake_hardware\' parameter is true.'
    )
    
    robot_ip_arg = DeclareLaunchArgument(
        'robot_ip',
        default_value='192.168.1.100',
        description='IP address of the robot server (only used if not using fake hardware)'
    )
    
    # Get launch configurations
    use_sim_time = LaunchConfiguration('use_sim_time')
    use_fake_hardware = LaunchConfiguration('use_fake_hardware')
    fake_sensor_commands = LaunchConfiguration('fake_sensor_commands')
    robot_ip = LaunchConfiguration('robot_ip')
    
    # Get package directory
    pkg_dir = get_package_share_directory('mobile_arm_manipulator_config')
    
    # Robot description
    robot_description_file = os.path.join(pkg_dir, 'config', 'mobile_arm_bot_2.urdf.xacro')
    robot_description = xacro.process_file(robot_description_file).toxml()
    
    # Real-time controller configuration
    ros2_controllers_file = os.path.join(pkg_dir, 'config', 'ros2_realtime_controllers.yaml')
    
    # Robot state publisher
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[
            {'robot_description': robot_description},
            {'use_sim_time': use_sim_time}
        ]
    )
    
    # Joint state publisher
    joint_state_publisher = Node(
        package='joint_state_publisher',
        executable='joint_state_publisher',
        name='joint_state_publisher',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )
    
    # Controller manager
    controller_manager = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[
            {'robot_description': robot_description},
            {'use_sim_time': use_sim_time},
            {'robot_ip': robot_ip},
            {'use_fake_hardware': use_fake_hardware},
            {'fake_sensor_commands': fake_sensor_commands}
        ],
        output='screen'
    )
    
    # Load real-time controllers
    load_realtime_controllers = ExecuteProcess(
        cmd=['ros2', 'control', 'load_controller', '--set-state', 'active', 
             'mobile_arm_realtime_controller'],
        output='screen'
    )
    
    load_gripper_controller = ExecuteProcess(
        cmd=['ros2', 'control', 'load_controller', '--set-state', 'active',
             'gripper_realtime_controller'],
        output='screen'
    )
    
    load_joint_state_broadcaster = ExecuteProcess(
        cmd=['ros2', 'control', 'load_controller', '--set-state', 'active',
             'joint_state_broadcaster'],
        output='screen'
    )
    
    # Delay controller loading
    delay_controllers = ExecuteProcess(
        cmd=['sleep', '2'],
        output='screen'
    )
    
    return LaunchDescription([
        use_sim_time_arg,
        use_fake_hardware_arg,
        fake_sensor_commands_arg,
        robot_ip_arg,
        robot_state_publisher,
        joint_state_publisher,
        controller_manager,
        delay_controllers,
        load_joint_state_broadcaster,
        load_realtime_controllers,
        load_gripper_controller
    ])
