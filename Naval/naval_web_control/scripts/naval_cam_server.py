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

from flask import Flask, Response, request, jsonify, render_template_string
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
        self.zoom_level = 1       # 1-5, 1=no zoom
        self.hw_zoom_works = None  # None=untested, True/False
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

    def _software_zoom(self, frame, level):
        """Apply software digital zoom by cropping center and scaling."""
        if level <= 1:
            return frame
        h, w = frame.shape[:2]
        # level 1=1.0x, 2=1.5x, 3=2.0x, 4=2.5x, 5=3.0x
        scale = 1.0 + (level - 1) * 0.5
        new_w = int(w / scale)
        new_h = int(h / scale)
        x1 = (w - new_w) // 2
        y1 = (h - new_h) // 2
        cropped = frame[y1:y1+new_h, x1:x1+new_w]
        return cv2.resize(cropped, (w, h), interpolation=cv2.INTER_LINEAR)

    def _capture(self):
        fail_count = 0
        while self.running:
            ret, frame = self.cap.read()
            if ret:
                fail_count = 0
                # Apply software zoom if hw zoom doesn't work
                if self.hw_zoom_works is False and self.zoom_level > 1:
                    frame = self._software_zoom(frame, self.zoom_level)
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

    def _test_hw_zoom(self):
        """Test if hardware zoom actually works on this camera."""
        if not self.cap or not self.cap.isOpened():
            return False
        # Set zoom to max, capture a frame, compare with zoom 0
        self.cap.set(cv2.CAP_PROP_ZOOM, 0)
        import time; time.sleep(0.5)
        for _ in range(5): self.cap.read()
        ret, f1 = self.cap.read()
        self.cap.set(cv2.CAP_PROP_ZOOM, 12)
        time.sleep(1)
        for _ in range(10): self.cap.read()
        ret, f2 = self.cap.read()
        self.cap.set(cv2.CAP_PROP_ZOOM, 0)
        if not ret or f1 is None or f2 is None:
            return False
        # Compare center regions - if zoomed, pixel values should differ
        h, w = f1.shape[:2]
        c1 = f1[h//3:2*h//3, w//3:2*w//3]
        c2 = f2[h//3:2*h//3, w//3:2*w//3]
        diff = cv2.absdiff(c1, c2).mean()
        works = diff > 15.0  # threshold for "actually changed" (lighting noise ~5-10)
        print(f"[{self.name}] HW zoom test: diff={diff:.1f}, works={works}")
        return works

    def set_zoom(self, level):
        """Set zoom level (1-5). Uses HW zoom if available, else software crop."""
        level = max(1, min(5, level))
        self.zoom_level = level

        # First time: test if hardware zoom works
        if self.hw_zoom_works is None:
            self.hw_zoom_works = self._test_hw_zoom()
            print(f"[{self.name}] Using {'hardware' if self.hw_zoom_works else 'software'} zoom")

        if self.hw_zoom_works:
            zoom_val = max(0, min(12, (level - 1) * 3))
            try:
                if self.cap and self.cap.isOpened():
                    self.cap.set(cv2.CAP_PROP_ZOOM, zoom_val)
                print(f"[{self.name}] HW zoom: {level} (val: {zoom_val})")
            except Exception as e:
                print(f"[{self.name}] HW zoom failed: {e}")
        else:
            print(f"[{self.name}] SW zoom: {level} ({1.0 + (level-1)*0.5:.1f}x)")

        return True


# --- Auto-detect cameras (retry up to 90s for USB enumeration) ---
cam_left = None
cam_right = None
devices = []

for attempt in range(18):  # 18 * 5s = 90s max wait
    print(f"Detecting OBSBOT cameras (attempt {attempt + 1})...")
    devices = find_obsbot_devices()
    if len(devices) >= 2:
        break
    print(f"  Found {len(devices)} camera(s), waiting 5s...")
    time.sleep(5)

print(f"Found devices: {devices}")

if len(devices) >= 2:
    cam_left = CameraStream(devices[0], "Left")
    cam_right = CameraStream(devices[1], "Right")
elif len(devices) == 1:
    cam_left = CameraStream(devices[0], "Left")
    print("WARNING: Only 1 camera found")
else:
    print("ERROR: No OBSBOT cameras found after retries")


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


@app.route('/tv')
def tv_dual_view():
    return render_template_string("""
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Naval HDMI Dual Camera</title>
  <style>
    :root {
      --bg: #0a0b0d;
      --panel: #12151a;
      --text: #e7edf7;
      --muted: #aab7cc;
      --line: #243145;
      --ok: #36d399;
      --warn: #f59e0b;
    }
    html, body {
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      background: var(--bg);
      color: var(--text);
      font-family: "DejaVu Sans", Arial, sans-serif;
      overflow: hidden;
    }
    .layout {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      padding: 10px;
      height: 100vh;
      box-sizing: border-box;
    }
    .cam {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 10px;
      display: grid;
      grid-template-rows: auto 1fr auto;
      overflow: hidden;
      min-height: 0;
    }
    .hdr {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 8px 12px;
      border-bottom: 1px solid var(--line);
      font-size: 18px;
      font-weight: 700;
      letter-spacing: 0.5px;
    }
    .st {
      font-size: 13px;
      color: var(--muted);
      font-weight: 600;
    }
    .frame-wrap {
      min-height: 0;
      background: #000;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    img {
      width: 100%;
      height: 100%;
      object-fit: contain;
      background: #000;
      user-select: none;
      -webkit-user-drag: none;
    }
    .ftr {
      border-top: 1px solid var(--line);
      padding: 6px 12px;
      font-size: 12px;
      color: var(--muted);
      display: flex;
      justify-content: space-between;
    }
    .ok { color: var(--ok); }
    .warn { color: var(--warn); }
  </style>
</head>
<body>
  <div class="layout">
    <section class="cam">
      <div class="hdr">
        <span>LEFT CAMERA</span>
        <span id="left-status" class="st">CONNECTING</span>
      </div>
      <div class="frame-wrap"><img id="left-img" alt="Left camera"></div>
      <div class="ftr"><span id="left-fps">-- FPS</span><span>/frame/left</span></div>
    </section>
    <section class="cam">
      <div class="hdr">
        <span>RIGHT CAMERA</span>
        <span id="right-status" class="st">CONNECTING</span>
      </div>
      <div class="frame-wrap"><img id="right-img" alt="Right camera"></div>
      <div class="ftr"><span id="right-fps">-- FPS</span><span>/frame/right</span></div>
    </section>
  </div>
  <script>
    const left = { img: document.getElementById('left-img'), st: document.getElementById('left-status'), fps: document.getElementById('left-fps') };
    const right = { img: document.getElementById('right-img'), st: document.getElementById('right-status'), fps: document.getElementById('right-fps') };

    function updateStatus(el, ok) {
      el.textContent = ok ? 'LIVE' : 'NO SIGNAL';
      el.className = 'st ' + (ok ? 'ok' : 'warn');
    }

    function startFeed(side, target) {
      let frames = 0;
      let lastFpsTs = performance.now();
      let active = true;

      async function tick() {
        if (!active) return;
        try {
          const resp = await fetch(`/frame/${side}?t=${Date.now()}`, { cache: 'no-store' });
          if (resp.ok) {
            const blob = await resp.blob();
            if (blob.size > 0) {
              const url = URL.createObjectURL(blob);
              target.img.onload = () => URL.revokeObjectURL(url);
              target.img.src = url;
              frames += 1;
              updateStatus(target.st, true);
            } else {
              updateStatus(target.st, false);
            }
          } else {
            updateStatus(target.st, false);
          }
        } catch (_) {
          updateStatus(target.st, false);
        }

        const now = performance.now();
        if (now - lastFpsTs >= 1000) {
          target.fps.textContent = `${frames} FPS`;
          frames = 0;
          lastFpsTs = now;
        }
        setTimeout(tick, 33);
      }

      tick();
      return () => { active = false; };
    }

    const stopLeft = startFeed('left', left);
    const stopRight = startFeed('right', right);
    window.addEventListener('beforeunload', () => { stopLeft(); stopRight(); });
  </script>
</body>
</html>
""")


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
