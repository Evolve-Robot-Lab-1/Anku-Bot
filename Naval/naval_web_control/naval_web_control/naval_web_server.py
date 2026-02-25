#!/usr/bin/env python3
"""
Naval Web Server
================
Flask web server for the Naval inspection robot control panel.
Serves HTML/JS/CSS and provides configuration endpoints.

Usage:
    ros2 run naval_web_control web_server
"""

import os
import threading
import rclpy
from rclpy.node import Node
from flask import Flask, render_template, send_from_directory, jsonify, request
try:
    from flask_cors import CORS
    HAS_CORS = True
except ImportError:
    HAS_CORS = False
from ament_index_python.packages import get_package_share_directory


class NavalWebServer(Node):
    """ROS2 Node that runs Flask web server."""

    def __init__(self):
        super().__init__('naval_web_server')

        # Parameters
        self.declare_parameter('port', 5000)
        self.declare_parameter('host', '0.0.0.0')
        self.declare_parameter('rosbridge_port', 9090)
        self.declare_parameter('cam_server_port', 8090)
        self.declare_parameter('cmd_vel_topic', '/cmd_vel')

        self.port = self.get_parameter('port').value
        self.host = self.get_parameter('host').value
        self.rosbridge_port = self.get_parameter('rosbridge_port').value
        self.cam_server_port = self.get_parameter('cam_server_port').value
        self.cmd_vel_topic = self.get_parameter('cmd_vel_topic').value

        # Get package share directory for static files
        try:
            self.pkg_share = get_package_share_directory('naval_web_control')
        except Exception:
            self.pkg_share = os.path.dirname(
                os.path.dirname(os.path.abspath(__file__)))

        self.get_logger().info(f'Package share: {self.pkg_share}')

        # Create Flask app
        self.app = Flask(
            __name__,
            template_folder=os.path.join(self.pkg_share, 'templates'),
            static_folder=os.path.join(self.pkg_share, 'static')
        )
        if HAS_CORS:
            CORS(self.app)

        # ROS publishers for speed/estop (bypass rosbridge)
        from std_msgs.msg import Int32, Bool
        self.speed_pub = self.create_publisher(Int32, '/naval/speed', 10)
        self.estop_pub = self.create_publisher(Bool, '/naval/estop', 10)
        self.tilt_front_pub = self.create_publisher(Int32, '/naval/tilt_front', 10)
        self.tilt_rear_pub = self.create_publisher(Int32, '/naval/tilt_rear', 10)
        self.obstacle_enable_pub = self.create_publisher(Bool, '/naval/obstacle_enabled', 10)

        self._setup_routes()

        self.get_logger().info(
            f'Web server starting on http://{self.host}:{self.port}')

    def _setup_routes(self):
        """Setup Flask routes."""

        @self.app.route('/')
        def index():
            return render_template(
                'index.html',
                rosbridge_port=self.rosbridge_port,
                cam_server_port=self.cam_server_port,
                cmd_vel_topic=self.cmd_vel_topic
            )

        @self.app.route('/static/<path:path>')
        def serve_static(path):
            return send_from_directory(self.app.static_folder, path)

        @self.app.route('/api/config')
        def get_config():
            return jsonify({
                'rosbridge_port': self.rosbridge_port,
                'cam_server_port': self.cam_server_port
            })

        @self.app.route('/api/speed', methods=['POST', 'OPTIONS'])
        def set_speed():
            if request.method == 'OPTIONS':
                return '', 204
            from std_msgs.msg import Int32
            data = request.get_json(silent=True) or {}
            level = int(data.get('level', 1))
            msg = Int32()
            msg.data = level
            self.speed_pub.publish(msg)
            return jsonify({'success': True, 'level': level})

        @self.app.route('/api/estop', methods=['POST', 'OPTIONS'])
        def set_estop():
            if request.method == 'OPTIONS':
                return '', 204
            from std_msgs.msg import Bool
            data = request.get_json(silent=True) or {}
            active = bool(data.get('active', False))
            msg = Bool()
            msg.data = active
            self.estop_pub.publish(msg)
            return jsonify({'success': True, 'active': active})

        @self.app.route('/api/tilt', methods=['POST', 'OPTIONS'])
        def set_tilt():
            if request.method == 'OPTIONS':
                return '', 204
            from std_msgs.msg import Int32
            data = request.get_json(silent=True) or {}
            side = data.get('side', 'front')
            angle = int(data.get('angle', 0))
            msg = Int32()
            msg.data = angle
            if side == 'front':
                self.tilt_front_pub.publish(msg)
            else:
                self.tilt_rear_pub.publish(msg)
            return jsonify({'success': True, 'side': side, 'angle': angle})

        @self.app.route('/api/obstacle', methods=['POST', 'OPTIONS'])
        def set_obstacle():
            if request.method == 'OPTIONS':
                return '', 204
            from std_msgs.msg import Bool
            data = request.get_json(silent=True) or {}
            enabled = bool(data.get('enabled', True))
            msg = Bool()
            msg.data = enabled
            self.obstacle_enable_pub.publish(msg)
            return jsonify({'success': True, 'enabled': enabled})

        @self.app.route('/health')
        def health():
            return jsonify({'status': 'ok'})

    def run_server(self):
        """Run Flask server in a separate thread."""
        self.app.run(
            host=self.host,
            port=self.port,
            debug=False,
            use_reloader=False,
            threaded=True
        )


def main(args=None):
    rclpy.init(args=args)
    node = NavalWebServer()

    server_thread = threading.Thread(target=node.run_server, daemon=True)
    server_thread.start()

    node.get_logger().info('Web server is running. Press Ctrl+C to stop.')

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
