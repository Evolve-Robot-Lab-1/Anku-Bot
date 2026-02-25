#!/usr/bin/env python3
"""
Naval Camera Server (Standalone)
=================================
Flask server for dual OBSBOT Meet SE cameras with zoom control.
Runs independently of ROS2 (survives ROS restarts).

Endpoints:
    GET  /frame/left   - JPEG frame from left camera
    GET  /frame/right  - JPEG frame from right camera
    POST /zoom/left    - Set zoom for left camera  (JSON: {"level": 1-5})
    POST /zoom/right   - Set zoom for right camera (JSON: {"level": 1-5})
    GET  /health       - Health check

Port: 8090
"""

from flask import Flask, Response, request, jsonify
import cv2
import threading
import subprocess
import re
import time

app = Flask(__name__)


def find_obsbot_devices():
    """Auto-detect OBSBOT Meet SE camera device numbers."""
    try:
        result = subprocess.run(
            ['v4l2-ctl', '--list-devices'],
            capture_output=True, text=True, timeout=5)
        output = result.stdout + result.stderr
        devices = []
        lines = output.split('\n')
        i = 0
        while i < len(lines):
            if 'OBSBOT' in lines[i]:
                while i + 1 < len(lines) and lines[i + 1].startswith('\t'):
                    i += 1
                    dev = lines[i].strip()
                    if re.match(r'/dev/video\d+$', dev):
                        devices.append(dev)
                        break
            i += 1
        return devices
    except Exception as e:
        print(f"Auto-detect failed: {e}")
        return []


class CameraStream:
    def __init__(self, src, name):
        self.name = name
        self.dev = src
        self.cap = None
        self.lock = threading.Lock()
        self.frame_buf = None
        self.running = True
        self._open_camera()
        self.thread = threading.Thread(target=self._capture, daemon=True)
        self.thread.start()

    def _open_camera(self):
        if self.cap is not None:
            self.cap.release()
        self.cap = cv2.VideoCapture(self.dev, cv2.CAP_V4L2)
        self.cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        self.cap.set(cv2.CAP_PROP_FPS, 30)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        print(f"[{self.name}] Opened {self.dev}")

    def _capture(self):
        fail_count = 0
        while self.running:
            ret, frame = self.cap.read()
            if ret:
                fail_count = 0
                ret, buf = cv2.imencode(
                    '.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
                if ret:
                    with self.lock:
                        self.frame_buf = buf.tobytes()
            else:
                fail_count += 1
                if fail_count > 30:
                    print(f"[{self.name}] Camera lost, trying to reopen...")
                    self._open_camera()
                    fail_count = 0

    def get_frame(self):
        with self.lock:
            return self.frame_buf

    def set_zoom(self, level):
        """Set zoom level (1-5) via v4l2-ctl."""
        # OBSBOT zoom_absolute typically ranges 100-500
        zoom_val = max(100, min(500, level * 100))
        try:
            subprocess.run(
                ['v4l2-ctl', '-d', self.dev,
                 '--set-ctrl', f'zoom_absolute={zoom_val}'],
                capture_output=True, timeout=2)
            print(f"[{self.name}] Zoom set to {level} (v4l2: {zoom_val})")
            return True
        except Exception as e:
            print(f"[{self.name}] Zoom failed: {e}")
            return False


# --- Auto-detect cameras ---
print("Detecting OBSBOT cameras...")
devices = find_obsbot_devices()
print(f"Found devices: {devices}")

cam_left = None
cam_right = None

if len(devices) >= 2:
    cam_left = CameraStream(devices[0], "Left")
    cam_right = CameraStream(devices[1], "Right")
elif len(devices) == 1:
    cam_left = CameraStream(devices[0], "Left")
    print("WARNING: Only 1 camera found")
else:
    print("ERROR: No OBSBOT cameras found")


# --- CORS helper ---
def add_cors(response):
    response.headers['Access-Control-Allow-Origin'] = '*'
    response.headers['Access-Control-Allow-Methods'] = 'GET, POST, OPTIONS'
    response.headers['Access-Control-Allow-Headers'] = 'Content-Type'
    return response

app.after_request(add_cors)


# --- Routes ---
@app.route('/frame/left')
def frame_left():
    if cam_left is None:
        return '', 204
    f = cam_left.get_frame()
    if f is None:
        return '', 204
    return Response(f, mimetype='image/jpeg',
                    headers={'Cache-Control': 'no-store'})


@app.route('/frame/right')
def frame_right():
    if cam_right is None:
        return '', 204
    f = cam_right.get_frame()
    if f is None:
        return '', 204
    return Response(f, mimetype='image/jpeg',
                    headers={'Cache-Control': 'no-store'})


@app.route('/zoom/left', methods=['POST', 'OPTIONS'])
def zoom_left():
    if request.method == 'OPTIONS':
        return '', 204
    if cam_left is None:
        return jsonify({'error': 'No left camera'}), 404
    data = request.get_json(silent=True) or {}
    level = data.get('level', 1)
    ok = cam_left.set_zoom(int(level))
    return jsonify({'success': ok, 'level': level})


@app.route('/zoom/right', methods=['POST', 'OPTIONS'])
def zoom_right():
    if request.method == 'OPTIONS':
        return '', 204
    if cam_right is None:
        return jsonify({'error': 'No right camera'}), 404
    data = request.get_json(silent=True) or {}
    level = data.get('level', 1)
    ok = cam_right.set_zoom(int(level))
    return jsonify({'success': ok, 'level': level})


@app.route('/health')
def health():
    return jsonify({
        'status': 'ok',
        'left_camera': cam_left is not None,
        'right_camera': cam_right is not None
    })


if __name__ == '__main__':
    from werkzeug.serving import WSGIRequestHandler
    WSGIRequestHandler.protocol_version = "HTTP/1.1"
    app.run(host='0.0.0.0', port=8090, threaded=True)
