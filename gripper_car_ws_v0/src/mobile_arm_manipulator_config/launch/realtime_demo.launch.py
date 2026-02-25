#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory
from moveit_configs_utils import MoveItConfigsBuilder

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
        description='Enable fake command interfaces for sensors used for simple simulations'
    )
    
    robot_ip_arg = DeclareLaunchArgument(
        'robot_ip',
        default_value='192.168.1.100',
        description='IP address of the robot server (only used if not using fake hardware)'
    )
    
    launch_rviz_arg = DeclareLaunchArgument(
        'launch_rviz',
        default_value='true',
        description='Launch RViz2 with MoveIt configuration'
    )
    
    # Get launch configurations
    use_sim_time = LaunchConfiguration('use_sim_time')
    use_fake_hardware = LaunchConfiguration('use_fake_hardware')
    fake_sensor_commands = LaunchConfiguration('fake_sensor_commands')
    robot_ip = LaunchConfiguration('robot_ip')
    launch_rviz = LaunchConfiguration('launch_rviz')
    
    # Build MoveIt configuration
    moveit_config = (
        MoveItConfigsBuilder("mobile_arm_bot_2", package_name="mobile_arm_manipulator_config")
        .robot_description(
            file_path=os.path.join(
                get_package_share_directory("mobile_arm_manipulator_config"),
                "config",
                "mobile_arm_bot_2.urdf.xacro",
            )
        )
        .robot_description_semantic(
            file_path=os.path.join(
                get_package_share_directory("mobile_arm_manipulator_config"),
                "config",
                "mobile_arm_bot_2.srdf",
            )
        )
        .trajectory_execution(
            file_path=os.path.join(
                get_package_share_directory("mobile_arm_manipulator_config"),
                "config",
                "moveit_controllers.yaml",
            )
        )
        .to_moveit_configs()
    )
    
    # Override with real-time controller configuration
    moveit_config.robot_description_kinematics = {
        "robot_ip": robot_ip,
        "use_fake_hardware": use_fake_hardware,
        "fake_sensor_commands": fake_sensor_commands,
    }
    
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
            'fake_sensor_commands': fake_sensor_commands,
            'robot_ip': robot_ip,
        }.items(),
    )
    
    # MoveIt move_group launch
    move_group_launch = IncludeLaunchDescription(
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
    
    # RViz launch (delayed to allow controllers to start)
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
        fake_sensor_commands_arg,
        robot_ip_arg,
        launch_rviz_arg,
        realtime_controllers_launch,
        move_group_launch,
        rviz_launch,
    ])
