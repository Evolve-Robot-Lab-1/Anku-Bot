#!/usr/bin/env python3
"""Serve the Raspberry Pi CSI camera as a small MJPEG feed over HTTP."""

import argparse
import json
import os
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


PAGE = b"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Robot live camera</title>
  <style>
    body { margin: 0; background: #111; color: #eee; font: 16px sans-serif; }
    main { max-width: 960px; margin: 0 auto; padding: 16px; }
    img { display: block; width: 100%; height: auto; background: #222; }
    a { color: #9bd; }
  </style>
</head>
<body>
  <main>
    <h1>Robot live camera</h1>
    <img src="/stream.mjpg" alt="Live view from the Raspberry Pi camera">
    <p><a href="/snapshot.jpg">Open current frame</a></p>
  </main>
</body>
</html>"""


class Feed:
    def __init__(self):
        self.condition = threading.Condition()
        self.jpeg = None
        self.sequence = 0
        self.captured_at = 0.0
        self.error = None

    def publish(self, jpeg):
        with self.condition:
            self.jpeg = jpeg
            self.sequence += 1
            self.captured_at = time.monotonic()
            self.condition.notify_all()

    def fail(self, error):
        with self.condition:
            self.error = str(error)
            self.condition.notify_all()


def capture_frames(process, feed):
    buffer = bytearray()
    try:
        while True:
            chunk = os.read(process.stdout.fileno(), 8192)
            if not chunk:
                raise RuntimeError(
                    f"rpicam-vid stopped (exit code {process.poll()})"
                )
            buffer.extend(chunk)
            while True:
                start = buffer.find(b"\xff\xd8")
                if start < 0:
                    buffer[:] = buffer[-1:]
                    break
                end = buffer.find(b"\xff\xd9", start + 2)
                if end < 0:
                    if start:
                        del buffer[:start]
                    if len(buffer) > 2_000_000:
                        raise RuntimeError("JPEG frame exceeds 2 MB")
                    break
                feed.publish(bytes(buffer[start:end + 2]))
                del buffer[:end + 2]
    except Exception as exc:
        feed.fail(exc)
        print(f"Camera capture stopped: {exc}", flush=True)


def make_handler(feed):
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path in ("/", "/index.html"):
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(PAGE)))
                self.end_headers()
                self.wfile.write(PAGE)
                return

            if self.path == "/health":
                with feed.condition:
                    status = {
                        "frames": feed.sequence,
                        "age_seconds": (
                            round(time.monotonic() - feed.captured_at, 2)
                            if feed.captured_at else None
                        ),
                        "error": feed.error,
                    }
                body = json.dumps(status).encode()
                self.send_response(200 if status["error"] is None else 503)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                self.wfile.write(body)
                return

            if self.path == "/snapshot.jpg":
                with feed.condition:
                    jpeg = feed.jpeg
                if jpeg is None:
                    self.send_error(503, "Camera has no frame yet")
                    return
                self.send_response(200)
                self.send_header("Content-Type", "image/jpeg")
                self.send_header("Content-Length", str(len(jpeg)))
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                self.wfile.write(jpeg)
                return

            if self.path == "/stream.mjpg":
                self.send_response(200)
                self.send_header(
                    "Content-Type", "multipart/x-mixed-replace; boundary=frame"
                )
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                sequence = 0
                try:
                    while True:
                        with feed.condition:
                            feed.condition.wait_for(
                                lambda: feed.sequence != sequence or feed.error,
                                timeout=5,
                            )
                            if feed.error:
                                break
                            if feed.sequence == sequence:
                                continue
                            sequence = feed.sequence
                            jpeg = feed.jpeg
                        self.wfile.write(
                            b"--frame\r\n"
                            b"Content-Type: image/jpeg\r\n"
                            + f"Content-Length: {len(jpeg)}\r\n\r\n".encode()
                            + jpeg
                            + b"\r\n"
                        )
                except (BrokenPipeError, ConnectionResetError):
                    pass
                return

            self.send_error(404)

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--fps", type=int, default=15)
    args = parser.parse_args()

    process = subprocess.Popen(
        [
            "rpicam-vid",
            "--nopreview",
            "--timeout", "0",
            "--width", str(args.width),
            "--height", str(args.height),
            "--framerate", str(args.fps),
            "--codec", "mjpeg",
            "--flush",
            "--output", "-",
        ],
        stdout=subprocess.PIPE,
        bufsize=0,
    )
    feed = Feed()
    worker = threading.Thread(
        target=capture_frames, args=(process, feed), daemon=True
    )
    worker.start()
    try:
        with feed.condition:
            feed.condition.wait_for(
                lambda: feed.sequence > 0 or feed.error is not None, timeout=10
            )
            if feed.sequence == 0:
                raise RuntimeError(feed.error or "Camera produced no frames")

        server = ThreadingHTTPServer((args.host, args.port), make_handler(feed))
        server.daemon_threads = True
        print(f"Camera feed ready at http://{args.host}:{args.port}/", flush=True)
        try:
            server.serve_forever()
        finally:
            server.server_close()
    finally:
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


if __name__ == "__main__":
    main()
