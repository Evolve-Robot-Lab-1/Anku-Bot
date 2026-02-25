"""
Pick and Place Application Launch
=================================
Launches just the pick-and-place coordinator node.
Use with robot_bringup/pick_and_place.launch.py for full system.
"""

import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    """Generate launch description for pick and place application."""

    pkg_share = get_package_share_directory('mobile_manipulator_application')
    params_file = os.path.join(pkg_share, 'config', 'pick_and_place_params.yaml')

    pick_and_place_node = Node(
        package='mobile_manipulator_application',
        executable='pick_and_place_node',
        name='pick_and_place',
        output='screen',
        parameters=[params_file],
    )

    return LaunchDescription([
        pick_and_place_node,
    ])
