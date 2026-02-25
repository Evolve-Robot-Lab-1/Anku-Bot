#!/usr/bin/env python3

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, ExecuteProcess, RegisterEventHandler, DeclareLaunchArgument
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, Command
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue
import xacro


def generate_launch_description():
    
    # Package directories
    mobile_base_description_dir = get_package_share_directory('mobile_manipulator_sim')
    navigation_sim_dir = get_package_share_directory('navigation_sim')
    gazebo_ros_pkg = get_package_share_directory('gazebo_ros')
    
    # Paths
    xacro_file = os.path.join(mobile_base_description_dir, 'urdf', 'mobile_manipulator.urdf.xacro')
    #world_file = os.path.join(mobile_base_description_dir, 'worlds', 'empty.world')
    slam_params_file = os.path.join(navigation_sim_dir, 'config', 'mapping_params.yaml')
    #rviz_config = os.path.join(mobile_base_description_dir, 'rviz', 'slam_simulation.rviz')
    
    # Process xacro to generate URDF
    robot_description_content = Command([
        'xacro ', xacro_file,
        ' use_sim:=true'  # Optional parameter to enable simulation-specific configs
    ])
    
    # Launch configuration variables
    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    x_pose = LaunchConfiguration('x_pose', default='0.0')
    y_pose = LaunchConfiguration('y_pose', default='0.0')
    z_pose = LaunchConfiguration('z_pose', default='0.1')
    
    # Declare launch arguments
    declare_use_sim_time_cmd = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')
    
    declare_x_position_cmd = DeclareLaunchArgument(
        'x_pose', default_value='0.0',
        description='Initial x position of robot')
    
    declare_y_position_cmd = DeclareLaunchArgument(
        'y_pose', default_value='0.0',
        description='Initial y position of robot')
    
    declare_z_position_cmd = DeclareLaunchArgument(
        'z_pose', default_value='0.1',
        description='Initial z position of robot')
    
    # Robot State Publisher
    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': ParameterValue(robot_description_content, value_type=str),
            'use_sim_time': use_sim_time
        }]
    )
    
    # Joint State Publisher
    joint_state_publisher_node = Node(
        package='joint_state_publisher',
        executable='joint_state_publisher',
        name='joint_state_publisher',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )
    
    # Gazebo Server (gzserver)
    start_gazebo_server = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gazebo_ros_pkg, 'launch', 'gzserver.launch.py')
        )
    )
    
    # Gazebo Client (gzclient)
    start_gazebo_client = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gazebo_ros_pkg, 'launch', 'gzclient.launch.py')
        )
    )
    
    # Spawn Robot in Gazebo
    spawn_robot_node = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=[
            '-topic', 'robot_description',
            '-entity', 'mobile_manipulator',
            '-x', x_pose,
            '-y', y_pose,
            '-z', z_pose
        ],
        output='screen'
    )
    
    # Differential Drive Controller for Gazebo (if needed)
    # This publishes odom and accepts cmd_vel
    diff_drive_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['diff_drive_controller'],
        output='screen'
    )
    
    # Joint State Broadcaster
    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster'],
        output='screen'
    )
    
    # Teleop Twist Keyboard for manual control
    teleop_node = Node(
        package='teleop_twist_keyboard',
        executable='teleop_twist_keyboard',
        name='teleop_twist_keyboard',
        output='screen',
        prefix='xterm -e',  # Opens in new terminal
        remappings=[('/cmd_vel', '/cmd_vel')]
    )
    
    # SLAM Toolbox - Online Async Mapping
    slam_toolbox_node = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            slam_params_file,
            {'use_sim_time': use_sim_time}
        ]
    )
    
    # RViz2 for visualization
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        parameters=[{'use_sim_time': use_sim_time}]
    )
    
    # Create launch description
    ld = LaunchDescription()
    
    # Declare arguments
    ld.add_action(declare_use_sim_time_cmd)
    ld.add_action(declare_x_position_cmd)
    ld.add_action(declare_y_position_cmd)
    ld.add_action(declare_z_position_cmd)
    
    # Add nodes in order
    ld.add_action(robot_state_publisher_node)
    ld.add_action(joint_state_publisher_node)
    ld.add_action(start_gazebo_server)
    ld.add_action(start_gazebo_client)
    ld.add_action(spawn_robot_node)
    ld.add_action(teleop_node)
    ld.add_action(slam_toolbox_node)
    ld.add_action(rviz_node)
    
    return ld
