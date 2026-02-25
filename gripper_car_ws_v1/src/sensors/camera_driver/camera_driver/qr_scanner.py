#!/usr/bin/env python3
"""
QR Code Scanner Node
====================
Subscribes to camera images and detects QR codes.
Publishes decoded QR data as string messages.

Topics:
  Subscribe:
    - /camera/image_raw (sensor_msgs/Image): Camera images

  Publish:
    - /qr_scanner/data (std_msgs/String): Decoded QR code data
    - /qr_scanner/detection (sensor_msgs/Image): Annotated detection image

Services:
    - /qr_scanner/scan (std_srvs/Trigger): Trigger single scan
"""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String
from cv_bridge import CvBridge

try:
    import cv2
    from pyzbar import pyzbar
    DEPS_AVAILABLE = True
except ImportError:
    DEPS_AVAILABLE = False


class QRScannerNode(Node):
    """QR code scanner node."""

    def __init__(self):
        super().__init__('qr_scanner')

        if not DEPS_AVAILABLE:
            self.get_logger().error(
                'Missing dependencies: opencv-python, pyzbar. '
                'Install with: pip3 install opencv-python pyzbar'
            )
            return

        # Parameters
        self.declare_parameter('camera_topic', '/camera/image_raw')
        self.declare_parameter('scan_rate', 10.0)

        camera_topic = self.get_parameter('camera_topic').value
        scan_rate = self.get_parameter('scan_rate').value

        # CV Bridge
        self.bridge = CvBridge()

        # State
        self.last_qr_data = None
        self.scan_enabled = True

        # Publishers
        self.pub_data = self.create_publisher(String, '/qr_scanner/data', 10)
        self.pub_detection = self.create_publisher(
            Image, '/qr_scanner/detection', 10)

        # Subscriber
        self.sub_image = self.create_subscription(
            Image, camera_topic, self.image_callback, 10)

        # Rate limiter
        self.scan_interval = 1.0 / scan_rate
        self.last_scan_time = self.get_clock().now()

        self.get_logger().info(f'QR Scanner initialized, listening to {camera_topic}')

    def image_callback(self, msg: Image):
        """Process camera images for QR codes."""
        if not self.scan_enabled:
            return

        # Rate limit
        current_time = self.get_clock().now()
        elapsed = (current_time - self.last_scan_time).nanoseconds / 1e9
        if elapsed < self.scan_interval:
            return
        self.last_scan_time = current_time

        try:
            # Convert ROS image to OpenCV
            cv_image = self.bridge.imgmsg_to_cv2(msg, 'bgr8')

            # Detect QR codes
            qr_codes = pyzbar.decode(cv_image)

            for qr in qr_codes:
                # Get QR data
                data = qr.data.decode('utf-8')

                # Only publish if different from last detection
                if data != self.last_qr_data:
                    self.last_qr_data = data

                    # Publish data
                    data_msg = String()
                    data_msg.data = data
                    self.pub_data.publish(data_msg)

                    self.get_logger().info(f'QR detected: {data}')

                # Draw bounding box on detection image
                points = qr.polygon
                if len(points) == 4:
                    pts = [(p.x, p.y) for p in points]
                    for i in range(4):
                        cv2.line(cv_image, pts[i], pts[(i + 1) % 4],
                                 (0, 255, 0), 2)

                    # Add text
                    cv2.putText(cv_image, data, (pts[0][0], pts[0][1] - 10),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            # Publish annotated image
            detection_msg = self.bridge.cv2_to_imgmsg(cv_image, 'bgr8')
            detection_msg.header = msg.header
            self.pub_detection.publish(detection_msg)

        except Exception as e:
            self.get_logger().warn(f'Error processing image: {e}')


def main(args=None):
    rclpy.init(args=args)

    if not DEPS_AVAILABLE:
        print('ERROR: Missing dependencies (opencv-python, pyzbar)')
        return

    node = QRScannerNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
