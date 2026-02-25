"""Launch file for mobile manipulator hardware interface."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    """Generate launch description for hardware node."""

    # Declare launch arguments
    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyACM0',
        description='Arduino serial port'
    )

    use_odom_tf_arg = DeclareLaunchArgument(
        'publish_odom_tf',
        default_value='true',
        description='Publish odom->base_link TF'
    )

    # Get package share directory
    pkg_share = FindPackageShare('mobile_manipulator_hardware')

    # Path to config file
    config_file = PathJoinSubstitution([
        pkg_share, 'config', 'hardware_params.yaml'
    ])

    # Hardware node
    hardware_node = Node(
        package='mobile_manipulator_hardware',
        executable='hardware_node',
        name='hardware_node',
        output='screen',
        parameters=[
            config_file,
            {
                'serial_port': LaunchConfiguration('serial_port'),
                'publish_odom_tf': LaunchConfiguration('publish_odom_tf'),
            }
        ],
        remappings=[
            # Remap topics if needed
        ]
    )

    return LaunchDescription([
        serial_port_arg,
        use_odom_tf_arg,
        hardware_node,
    ])
