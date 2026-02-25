#!/usr/bin/env python3
"""
Web Control Panel Server
========================
Flask-based web server for robot control panel.
Serves static files and provides REST API endpoints.

Usage:
    ros2 run web_control_panel web_server
"""

import os
import threading
import rclpy
from rclpy.node import Node
from flask import Flask, render_template, send_from_directory, jsonify
try:
    from flask_cors import CORS
    HAS_CORS = True
except ImportError:
    HAS_CORS = False
from ament_index_python.packages import get_package_share_directory


class WebServerNode(Node):
    """ROS2 Node that runs Flask web server."""

    def __init__(self):
        super().__init__('web_server')

        # Parameters
        self.declare_parameter('port', 5000)
        self.declare_parameter('host', '0.0.0.0')
        self.declare_parameter('rosbridge_port', 9090)
        self.declare_parameter('video_server_port', 8080)

        self.port = self.get_parameter('port').value
        self.host = self.get_parameter('host').value
        self.rosbridge_port = self.get_parameter('rosbridge_port').value
        self.video_server_port = self.get_parameter('video_server_port').value

        # Get package share directory for static files
        try:
            self.pkg_share = get_package_share_directory('web_control_panel')
        except Exception:
            # Fallback to source directory during development
            self.pkg_share = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

        self.get_logger().info(f'Package share directory: {self.pkg_share}')

        # Create Flask app
        self.app = Flask(
            __name__,
            template_folder=os.path.join(self.pkg_share, 'templates'),
            static_folder=os.path.join(self.pkg_share, 'static')
        )
        if HAS_CORS:
            CORS(self.app)

        # Setup routes
        self._setup_routes()

        self.get_logger().info(f'Web server starting on http://{self.host}:{self.port}')

    def _setup_routes(self):
        """Setup Flask routes."""

        @self.app.route('/')
        def index():
            """Serve main control panel page."""
            return render_template(
                'index.html',
                rosbridge_port=self.rosbridge_port,
                video_server_port=self.video_server_port
            )

        @self.app.route('/static/<path:path>')
        def serve_static(path):
            """Serve static files."""
            return send_from_directory(self.app.static_folder, path)

        @self.app.route('/api/config')
        def get_config():
            """Return server configuration."""
            return jsonify({
                'rosbridge_port': self.rosbridge_port,
                'video_server_port': self.video_server_port,
                'camera_topic': '/camera/image_raw'
            })

        @self.app.route('/health')
        def health():
            """Health check endpoint."""
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

    node = WebServerNode()

    # Run Flask in a separate thread
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
