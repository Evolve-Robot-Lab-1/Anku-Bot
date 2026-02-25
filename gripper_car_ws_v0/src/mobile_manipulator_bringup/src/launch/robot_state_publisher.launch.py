#!/usr/bin/env python3

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.substitutions import PythonExpression
from launch_ros.actions import Node


def generate_launch_description():
    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    frame_prefix = LaunchConfiguration('frame_prefix', default='')

    urdf_path = os.path.join(
        get_package_share_directory('mobile_manipulator_bringup'),
        'urdf',
        'manipulator_mobile_with_arm.urdf')

    # Read the URDF file
    with open(urdf_path, 'r') as infp:
        robot_description_config = infp.read()

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='false',
            description='Use simulation (Gazebo) clock if true'),
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{
                'use_sim_time': use_sim_time,
                'robot_description': robot_description_config,
                'frame_prefix': frame_prefix
            }],
        ),
    ])