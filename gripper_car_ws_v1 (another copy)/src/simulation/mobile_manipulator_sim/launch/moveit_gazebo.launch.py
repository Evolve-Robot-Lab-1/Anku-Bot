#!/usr/bin/env python3
"""
MoveIt Launch for Gazebo Simulation
====================================
Launches MoveIt move_group using simulation-specific config
with static_ prefixed joint names.
"""

import os
import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory


def load_yaml(package_name, file_path):
    package_path = get_package_share_directory(package_name)
    absolute_file_path = os.path.join(package_path, file_path)
    try:
        with open(absolute_file_path, 'r') as file:
            return yaml.safe_load(file)
    except Exception as e:
        print(f"Error loading {absolute_file_path}: {e}")
        return {}


def generate_launch_description():
    # Get package directories
    pkg_sim = get_package_share_directory('mobile_manipulator_sim')
    pkg_arm_moveit = get_package_share_directory('arm_moveit')

    use_sim_time = LaunchConfiguration('use_sim_time')

    # Robot description from simulation URDF
    urdf_file = os.path.join(pkg_sim, 'urdf', 'mobile_manipulator.urdf.xacro')
    robot_description = ParameterValue(
        Command(['xacro ', urdf_file, ' sim_mode:=true']),
        value_type=str
    )

    # Load SRDF for simulation
    srdf_file = os.path.join(pkg_sim, 'config', 'sim_moveit.srdf')
    with open(srdf_file, 'r') as f:
        robot_description_semantic = f.read()

    # Load kinematics from arm_moveit (joint names don't matter for kinematics solver)
    kinematics_yaml = load_yaml('arm_moveit', 'config/kinematics.yaml')

    # Load joint limits from arm_moveit
    joint_limits_yaml = load_yaml('arm_moveit', 'config/joint_limits.yaml')

    # Load MoveIt controllers for simulation
    moveit_controllers = load_yaml('mobile_manipulator_sim', 'config/sim_moveit_controllers.yaml')

    # Planning pipeline - use OMPL
    ompl_planning = load_yaml('arm_moveit', 'config/ompl_planning.yaml')
    planning_pipelines = {'planning_pipelines': ['ompl'], 'default_planning_pipeline': 'ompl'}
    planning_pipelines['ompl'] = ompl_planning

    # Move Group Node configuration
    move_group_params = {
        'robot_description': robot_description,
        'robot_description_semantic': robot_description_semantic,
        'robot_description_kinematics': kinematics_yaml,
        'robot_description_planning': joint_limits_yaml,
        'use_sim_time': use_sim_time,
        'publish_robot_description_semantic': True,
        'allow_trajectory_execution': True,
        'capabilities': '',
        'disable_capabilities': '',
        'publish_planning_scene': True,
        'publish_geometry_updates': True,
        'publish_state_updates': True,
        'publish_transforms_updates': True,
        'monitor_dynamics': False,
    }
    move_group_params.update(moveit_controllers)
    move_group_params.update(planning_pipelines)

    # Move Group Node
    move_group_node = Node(
        package="moveit_ros_move_group",
        executable="move_group",
        output="screen",
        parameters=[move_group_params],
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use simulation time'
        ),
        move_group_node,
    ])
