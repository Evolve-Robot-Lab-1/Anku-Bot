# Copyright 2019 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def spawn_robot(context, *args, **kwargs):
    """Function to spawn robot based on use_sdf parameter"""
    pkg_share = get_package_share_directory('mobile_manipulator_bringup')

    # Get launch configurations
    x_pose = LaunchConfiguration('x_pose').perform(context)
    y_pose = LaunchConfiguration('y_pose').perform(context)
    z_pose = LaunchConfiguration('z_pose').perform(context)
    use_sdf = LaunchConfiguration('use_sdf').perform(context)

    # Determine spawn method
    if use_sdf.lower() == 'true':
        # Spawn from SDF model file
        model_path = os.path.join(pkg_share, 'models', 'mobile_manipulator', 'model.sdf')
        spawn_args = [
            '-entity', 'mobile_manipulator',
            '-file', model_path,
            '-x', x_pose,
            '-y', y_pose,
            '-z', z_pose
        ]
    else:
        # Spawn from URDF via robot_description topic
        spawn_args = [
            '-entity', 'mobile_manipulator',
            '-topic', 'robot_description',
            '-x', x_pose,
            '-y', y_pose,
            '-z', z_pose
        ]

    spawn_node = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=spawn_args,
        output='screen',
    )

    return [spawn_node]


def generate_launch_description():
    # Declare the launch arguments
    declare_x_position_cmd = DeclareLaunchArgument(
        'x_pose', default_value='0.0',
        description='Specify the X coordinate for the robot spawn position')

    declare_y_position_cmd = DeclareLaunchArgument(
        'y_pose', default_value='0.0',
        description='Specify the Y coordinate for the robot spawn position')

    declare_z_position_cmd = DeclareLaunchArgument(
        'z_pose', default_value='0.1',
        description='Specify the Z coordinate for the robot spawn position')

    declare_use_sdf_cmd = DeclareLaunchArgument(
        'use_sdf', default_value='true',
        description='If true, spawn from SDF model file; if false, spawn from URDF via robot_description topic')

    # Use OpaqueFunction to decide spawn method at runtime
    spawn_robot_cmd = OpaqueFunction(function=spawn_robot)

    ld = LaunchDescription()

    # Declare the launch options
    ld.add_action(declare_x_position_cmd)
    ld.add_action(declare_y_position_cmd)
    ld.add_action(declare_z_position_cmd)
    ld.add_action(declare_use_sdf_cmd)

    # Spawn robot
    ld.add_action(spawn_robot_cmd)

    return ld
