from flask import Flask, Response
import cv2
import threading
import subprocess
import re

app = Flask(__name__)

def find_obsbot_devices():
    """Auto-detect OBSBOT Meet SE camera device numbers."""
    try:
        result = subprocess.run(['v4l2-ctl', '--list-devices'],
                                capture_output=True, text=True, timeout=5)
        output = result.stdout + result.stderr
        devices = []
        lines = output.split('\n')
        i = 0
        while i < len(lines):
            if 'OBSBOT' in lines[i]:
                usb_path = lines[i]
                while i + 1 < len(lines) and lines[i + 1].startswith('\t'):
                    i += 1
                    dev = lines[i].strip()
                    if re.match(r'/dev/video\d+$', dev):
                        # Take only the first /dev/videoX per camera (capture device)
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
                ret, buf = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
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

# Auto-detect cameras
print("Detecting OBSBOT cameras...")
devices = find_obsbot_devices()
print(f"Found devices: {devices}")

if len(devices) >= 2:
    cam_left = CameraStream(devices[0], "Left")
    cam_right = CameraStream(devices[1], "Right")
elif len(devices) == 1:
    cam_left = CameraStream(devices[0], "Left")
    cam_right = None
    print("WARNING: Only 1 camera found")
else:
    cam_left = None
    cam_right = None
    print("ERROR: No OBSBOT cameras found")

HTML = """<!DOCTYPE html>
<html><head><title>OBSBOT Dual Camera - RPi</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;}
body{background:#111;display:flex;flex-direction:column;height:100vh;font-family:monospace;color:#fff;}
.container{display:flex;flex:1;gap:4px;padding:4px;}
.cam{flex:1;position:relative;display:flex;justify-content:center;align-items:center;background:#000;border-radius:4px;overflow:hidden;}
.cam img{max-width:100%%;max-height:100%%;}
.label{position:absolute;top:8px;left:8px;background:rgba(0,0,0,0.7);padding:4px 10px;border-radius:4px;font-size:14px;color:#0f0;}
.fps{position:absolute;top:8px;right:8px;background:rgba(0,0,0,0.7);padding:4px 10px;border-radius:4px;font-size:14px;color:#0f0;}
</style></head><body>
<div class="container">
    <div class="cam">
        <div class="label">LEFT CAM</div>
        <div class="fps" id="fps1">-- FPS</div>
        <img id="left">
    </div>
    <div class="cam">
        <div class="label">RIGHT CAM</div>
        <div class="fps" id="fps2">-- FPS</div>
        <img id="right">
    </div>
</div>
<script>
function startFeed(imgId, url, fpsId) {
    var img = document.getElementById(imgId);
    var fpsEl = document.getElementById(fpsId);
    var frames = 0, lastTime = Date.now();
    function update() {
        var n = new Image();
        n.onload = function() {
            img.src = n.src;
            frames++;
            var now = Date.now();
            if (now - lastTime >= 1000) {
                fpsEl.textContent = frames + " FPS";
                frames = 0;
                lastTime = now;
            }
            requestAnimationFrame(update);
        };
        n.onerror = function() { setTimeout(update, 50); };
        n.src = url + "?t=" + Date.now();
    }
    update();
}
startFeed("left", "/frame/left", "fps1");
startFeed("right", "/frame/right", "fps2");
</script>
</body></html>"""

@app.route('/')
def index():
    return HTML

@app.route('/frame/left')
def frame_left():
    if cam_left is None:
        return '', 204
    f = cam_left.get_frame()
    if f is None:
        return '', 204
    return Response(f, mimetype='image/jpeg',
                    headers={'Cache-Control': 'no-store', 'Connection': 'keep-alive'})

@app.route('/frame/right')
def frame_right():
    if cam_right is None:
        return '', 204
    f = cam_right.get_frame()
    if f is None:
        return '', 204
    return Response(f, mimetype='image/jpeg',
                    headers={'Cache-Control': 'no-store', 'Connection': 'keep-alive'})

if __name__ == '__main__':
    from werkzeug.serving import WSGIRequestHandler
    WSGIRequestHandler.protocol_version = "HTTP/1.1"
    app.run(host='0.0.0.0', port=8090, threaded=True)
