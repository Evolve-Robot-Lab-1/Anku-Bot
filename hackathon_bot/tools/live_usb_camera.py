#!/usr/bin/env python3
"""Serve a USB webcam as an MJPEG feed."""

import argparse
import threading
import time
from http.server import ThreadingHTTPServer

import cv2

from live_camera import Feed, make_handler


def capture(camera, feed, stopped):
    try:
        while not stopped.is_set():
            ok, frame = camera.read()
            if not ok:
                raise RuntimeError("USB camera stopped returning frames")
            ok, jpeg = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
            if not ok:
                raise RuntimeError("JPEG encoding failed")
            feed.publish(jpeg.tobytes())
    except Exception as exc:
        feed.fail(exc)
    finally:
        camera.release()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", default="/dev/video0")
    parser.add_argument("--host", default="192.168.50.62")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()
    camera = cv2.VideoCapture(args.device, cv2.CAP_V4L2)
    if not camera.isOpened():
        raise RuntimeError(f"Cannot open {args.device}")
    camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    camera.set(cv2.CAP_PROP_FPS, 15)
    feed = Feed()
    stopped = threading.Event()
    worker = threading.Thread(target=capture, args=(camera, feed, stopped), daemon=True)
    worker.start()
    try:
        with feed.condition:
            feed.condition.wait_for(lambda: feed.sequence or feed.error, timeout=10)
            if not feed.sequence:
                raise RuntimeError(feed.error or "Camera produced no frames")
        server = ThreadingHTTPServer((args.host, args.port), make_handler(feed))
        server.daemon_threads = True
        print(f"USB camera ready at http://{args.host}:{args.port}/", flush=True)
        try:
            server.serve_forever()
        finally:
            server.server_close()
    finally:
        stopped.set()
        worker.join(timeout=3)


if __name__ == "__main__":
    main()
