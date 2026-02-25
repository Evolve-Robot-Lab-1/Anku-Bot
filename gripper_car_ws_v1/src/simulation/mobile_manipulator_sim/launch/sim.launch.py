#!/usr/bin/env python3
"""
Mobile Manipulator Unified Simulation Launch
=============================================

Launches the full mobile manipulator in Gazebo with optional:
- SLAM Toolbox for mapping
- Nav2 for autonomous navigation
- MoveIt2 for arm control
- Teleop for manual control

Usage:
    # SLAM mode (mapping)
    ros2 launch mobile_manipulator_sim sim.launch.py slam:=true

    # Navigation mode (with saved map)
    ros2 launch mobile_manipulator_sim sim.launch.py slam:=false map:=/path/to/map.yaml

    # MoveIt only (arm testing)
    ros2 launch mobile_manipulator_sim sim.launch.py use_nav:=false use_moveit:=true

    # Full stack
    ros2 launch mobile_manipulator_sim sim.launch.py use_nav:=true use_moveit:=true

    # Different world
    ros2 launch mobile_manipulator_sim sim.launch.py world:=qr_test_arena
"""

import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    TimerAction,
    GroupAction,
    ExecuteProcess,
    SetEnvironmentVariable,
)
from launch.substitutions import (
    LaunchConfiguration,
    Command,
    PythonExpression,
    PathJoinSubstitution,
)
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    # ==================== PACKAGE DIRECTORIES ====================
    pkg_sim = get_package_share_directory('mobile_manipulator_sim')
    pkg_gazebo = get_package_share_directory('mobile_base_gazebo')
    pkg_nav = get_package_share_directory('mobile_manipulator_navigation')
    pkg_moveit = get_package_share_directory('arm_moveit')
    pkg_gazebo_ros = get_package_share_directory('gazebo_ros')
    pkg_arm_description = get_package_share_directory('arm_description')

    # Build GAZEBO_MODEL_PATH to include arm meshes
    gazebo_model_path = os.path.join(pkg_gazebo, 'models')
    # Add arm_description parent directory so model://arm_description works
    arm_description_parent = os.path.dirname(pkg_arm_description)
    current_model_path = os.environ.get('GAZEBO_MODEL_PATH', '')
    full_model_path = ':'.join(filter(None, [gazebo_model_path, arm_description_parent, current_model_path]))

    # ==================== LAUNCH CONFIGURATIONS ====================
    slam = LaunchConfiguration('slam')
    use_nav = LaunchConfiguration('use_nav')
    use_moveit = LaunchConfiguration('use_moveit')
    use_rviz = LaunchConfiguration('use_rviz')
    use_teleop = LaunchConfiguration('use_teleop')
    world_name = LaunchConfiguration('world')
    map_yaml = LaunchConfiguration('map')
    x_pose = LaunchConfiguration('x')
    y_pose = LaunchConfiguration('y')
    z_pose = LaunchConfiguration('z')
    yaw = LaunchConfiguration('yaw')

    # ==================== FILE PATHS ====================
    urdf_file = os.path.join(pkg_sim, 'urdf', 'mobile_manipulator.urdf.xacro')
    nav2_params = os.path.join(pkg_nav, 'config', 'nav2_params.yaml')
    slam_params = os.path.join(pkg_nav, 'config', 'slam_params.yaml')
    rviz_config = os.path.join(pkg_sim, 'config', 'sim.rviz')

    # Robot description (sim_mode=true to use Gazebo ros2_control)
    robot_description = ParameterValue(
        Command(['xacro ', urdf_file, ' sim_mode:=true']),
        value_type=str
    )

    return LaunchDescription([
        # ==================== ENVIRONMENT ====================
        SetEnvironmentVariable('GAZEBO_MODEL_PATH', full_model_path),

        # ==================== ARGUMENTS ====================

        DeclareLaunchArgument(
            'slam',
            default_value='true',
            description='Enable SLAM mode for mapping (false = localization with map)'
        ),

        DeclareLaunchArgument(
            'use_nav',
            default_value='true',
            description='Enable Nav2 navigation stack'
        ),

        DeclareLaunchArgument(
            'use_moveit',
            default_value='false',
            description='Enable MoveIt2 for arm control'
        ),

        DeclareLaunchArgument(
            'use_rviz',
            default_value='true',
            description='Launch RViz for visualization'
        ),

        DeclareLaunchArgument(
            'use_teleop',
            default_value='true',
            description='Launch teleop keyboard control'
        ),

        DeclareLaunchArgument(
            'world',
            default_value='test_arena',
            description='Gazebo world name (without .world extension)'
        ),

        DeclareLaunchArgument(
            'map',
            default_value='',
            description='Path to map YAML file (required if slam:=false)'
        ),

        DeclareLaunchArgument(
            'x',
            default_value='-3.0',
            description='Initial X position'
        ),

        DeclareLaunchArgument(
            'y',
            default_value='-3.0',
            description='Initial Y position'
        ),

        DeclareLaunchArgument(
            'z',
            default_value='0.1',
            description='Initial Z position'
        ),

        DeclareLaunchArgument(
            'yaw',
            default_value='0.0',
            description='Initial yaw angle'
        ),

        # ==================== ROBOT STATE PUBLISHER ====================
        # Must start BEFORE Gazebo so gazebo_ros2_control can fetch robot_description

        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{
                'robot_description': robot_description,
                'use_sim_time': True,
            }]
        ),

        # ==================== GAZEBO ====================
        # Delay Gazebo start to ensure robot_state_publisher is ready

        TimerAction(
            period=2.0,
            actions=[
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource([
                        os.path.join(pkg_gazebo_ros, 'launch', 'gazebo.launch.py')
                    ]),
                    launch_arguments={
                        'world': PathJoinSubstitution([
                            pkg_gazebo, 'worlds', PythonExpression(["'", world_name, "' + '.world'"])
                        ]),
                        'verbose': 'true',
                    }.items(),
                ),
            ]
        ),

        # ==================== SPAWN ROBOT ====================
        # Delay after Gazebo starts (2s Gazebo + 5s wait = 7s total)

        TimerAction(
            period=7.0,
            actions=[
                Node(
                    package='gazebo_ros',
                    executable='spawn_entity.py',
                    name='spawn_entity',
                    output='screen',
                    arguments=[
                        '-entity', 'mobile_manipulator',
                        '-topic', 'robot_description',
                        '-x', x_pose,
                        '-y', y_pose,
                        '-z', z_pose,
                        '-Y', yaw,
                    ]
                ),
            ]
        ),

        # ==================== SLAM (MAPPING MODE) ====================

        TimerAction(
            period=8.0,
            actions=[
                Node(
                    package='slam_toolbox',
                    executable='async_slam_toolbox_node',
                    name='slam_toolbox',
                    output='screen',
                    parameters=[
                        slam_params,
                        {'use_sim_time': True}
                    ],
                    condition=IfCondition(
                        PythonExpression(["'", slam, "' == 'true' and '", use_nav, "' == 'true'"])
                    )
                ),
            ]
        ),

        # ==================== LOCALIZATION (MAP MODE) ====================

        # Map Server
        TimerAction(
            period=8.0,
            actions=[
                Node(
                    package='nav2_map_server',
                    executable='map_server',
                    name='map_server',
                    output='screen',
                    parameters=[
                        {'yaml_filename': map_yaml},
                        {'use_sim_time': True}
                    ],
                    condition=IfCondition(
                        PythonExpression(["'", slam, "' == 'false' and '", use_nav, "' == 'true'"])
                    )
                ),
            ]
        ),

        # AMCL
        TimerAction(
            period=8.0,
            actions=[
                Node(
                    package='nav2_amcl',
                    executable='amcl',
                    name='amcl',
                    output='screen',
                    parameters=[
                        nav2_params,
                        {'use_sim_time': True}
                    ],
                    condition=IfCondition(
                        PythonExpression(["'", slam, "' == 'false' and '", use_nav, "' == 'true'"])
                    )
                ),
            ]
        ),

        # ==================== NAV2 STACK ====================

        # Controller Server
        TimerAction(
            period=10.0,
            actions=[
                Node(
                    package='nav2_controller',
                    executable='controller_server',
                    name='controller_server',
                    output='screen',
                    parameters=[nav2_params, {'use_sim_time': True}],
                    condition=IfCondition(use_nav)
                ),
            ]
        ),

        # Planner Server
        TimerAction(
            period=10.0,
            actions=[
                Node(
                    package='nav2_planner',
                    executable='planner_server',
                    name='planner_server',
                    output='screen',
                    parameters=[nav2_params, {'use_sim_time': True}],
                    condition=IfCondition(use_nav)
                ),
            ]
        ),

        # Behavior Server
        TimerAction(
            period=10.0,
            actions=[
                Node(
                    package='nav2_behaviors',
                    executable='behavior_server',
                    name='behavior_server',
                    output='screen',
                    parameters=[nav2_params, {'use_sim_time': True}],
                    condition=IfCondition(use_nav)
                ),
            ]
        ),

        # BT Navigator
        TimerAction(
            period=10.0,
            actions=[
                Node(
                    package='nav2_bt_navigator',
                    executable='bt_navigator',
                    name='bt_navigator',
                    output='screen',
                    parameters=[nav2_params, {'use_sim_time': True}],
                    condition=IfCondition(use_nav)
                ),
            ]
        ),

        # Lifecycle Manager for Navigation
        TimerAction(
            period=14.0,
            actions=[
                Node(
                    package='nav2_lifecycle_manager',
                    executable='lifecycle_manager',
                    name='lifecycle_manager_navigation',
                    output='screen',
                    parameters=[{
                        'use_sim_time': True,
                        'autostart': True,
                        'node_names': [
                            'controller_server',
                            'planner_server',
                            'behavior_server',
                            'bt_navigator',
                        ]
                    }],
                    condition=IfCondition(use_nav)
                ),
            ]
        ),

        # Lifecycle Manager for Localization (map mode only)
        TimerAction(
            period=12.0,
            actions=[
                Node(
                    package='nav2_lifecycle_manager',
                    executable='lifecycle_manager',
                    name='lifecycle_manager_localization',
                    output='screen',
                    parameters=[{
                        'use_sim_time': True,
                        'autostart': True,
                        'node_names': ['map_server', 'amcl']
                    }],
                    condition=IfCondition(
                        PythonExpression(["'", slam, "' == 'false' and '", use_nav, "' == 'true'"])
                    )
                ),
            ]
        ),

        # ==================== MOVEIT2 ====================
        # Use custom launch that doesn't start its own ros2_control_node
        # (Gazebo's gazebo_ros2_control handles hardware)

        TimerAction(
            period=12.0,
            actions=[
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource([
                        os.path.join(pkg_sim, 'launch', 'moveit_gazebo.launch.py')
                    ]),
                    launch_arguments={
                        'use_sim_time': 'true',
                    }.items(),
                    condition=IfCondition(use_moveit)
                ),
            ]
        ),

        # ==================== TELEOP ====================

        TimerAction(
            period=5.0,
            actions=[
                ExecuteProcess(
                    cmd=[
                        'xterm', '-e',
                        'ros2', 'run', 'teleop_twist_keyboard', 'teleop_twist_keyboard',
                        '--ros-args', '-p', 'use_sim_time:=true',
                        '-p', 'speed:=0.3', '-p', 'turn:=0.5'
                    ],
                    output='screen',
                    condition=IfCondition(use_teleop)
                ),
            ]
        ),

        # ==================== RVIZ ====================

        TimerAction(
            period=4.0,
            actions=[
                Node(
                    package='rviz2',
                    executable='rviz2',
                    name='rviz2',
                    output='screen',
                    arguments=['-d', rviz_config],
                    parameters=[{'use_sim_time': True}],
                    condition=IfCondition(use_rviz)
                ),
            ]
        ),
    ])
