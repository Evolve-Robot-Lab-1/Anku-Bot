"""
Launch file for Naval Inspection Robot web control panel.

Launches:
  1. rosbridge_websocket (port 9090)
  2. naval_serial_bridge (serial -> Arduino)
  3. naval_web_server (Flask on port 5000)

Camera server is started separately via start_naval.sh (standalone, no ROS2).
"""

import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_share = get_package_share_directory('naval_web_control')
    params_file = os.path.join(pkg_share, 'config', 'naval_params.yaml')

    return LaunchDescription([
        # ROSBridge WebSocket server
        Node(
            package='rosbridge_server',
            executable='rosbridge_websocket',
            name='rosbridge_websocket',
            parameters=[{'port': 9090}],
            output='screen'
        ),

        # Serial bridge to Arduino
        Node(
            package='naval_web_control',
            executable='serial_bridge',
            name='naval_serial_bridge',
            parameters=[params_file],
            output='screen'
        ),

        # Flask web server
        Node(
            package='naval_web_control',
            executable='web_server',
            name='naval_web_server',
            parameters=[params_file],
            output='screen'
        ),
    ])
