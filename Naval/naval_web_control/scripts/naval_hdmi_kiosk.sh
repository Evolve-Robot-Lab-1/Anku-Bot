#!/bin/bash
# Naval HDMI Kiosk Launcher
# Opens dual-camera TV view in kiosk mode when a local display session exists.

set -u

URL="${NAVAL_HDMI_URL:-http://127.0.0.1:8090/tv?kiosk=1}"
HEALTH_URL="${NAVAL_CAM_HEALTH_URL:-http://127.0.0.1:8090/health}"
LOG_TAG="[naval-hdmi-kiosk]"

find_browser() {
    if command -v chromium-browser >/dev/null 2>&1; then
        echo "chromium-browser"
        return 0
    fi
    if command -v chromium >/dev/null 2>&1; then
        echo "chromium"
        return 0
    fi
    if command -v google-chrome >/dev/null 2>&1; then
        echo "google-chrome"
        return 0
    fi
    return 1
}

if ! command -v curl >/dev/null 2>&1; then
    echo "$LOG_TAG curl not found, skipping HDMI kiosk."
    exit 0
fi

# Wait up to 60s for camera server to become healthy.
for _ in $(seq 1 60); do
    if curl -sf "$HEALTH_URL" >/dev/null 2>&1; then
        break
    fi
    sleep 1
done

BROWSER_CMD="$(find_browser || true)"
if [[ -z "$BROWSER_CMD" ]]; then
    echo "$LOG_TAG Chromium not installed, skipping HDMI kiosk."
    exit 0
fi

# Support local desktop sessions using X11 or Wayland.
if [[ -z "${DISPLAY:-}" ]] && [[ -S /tmp/.X11-unix/X0 ]]; then
    export DISPLAY=:0
fi
if [[ -z "${XDG_RUNTIME_DIR:-}" ]] && [[ -d "/run/user/$(id -u)" ]]; then
    export XDG_RUNTIME_DIR="/run/user/$(id -u)"
fi

if [[ -z "${DISPLAY:-}" ]] && [[ -z "${WAYLAND_DISPLAY:-}" ]]; then
    echo "$LOG_TAG No display session detected, skipping HDMI kiosk."
    exit 0
fi

# Kill prior kiosk window from older runs.
pkill -f "8090/tv\\?kiosk=1" 2>/dev/null || true
sleep 0.3

echo "$LOG_TAG Launching dual-camera HDMI view: $URL"
exec "$BROWSER_CMD" \
    --kiosk \
    --incognito \
    --noerrdialogs \
    --disable-infobars \
    --disable-session-crashed-bubble \
    --check-for-update-interval=31536000 \
    --overscroll-history-navigation=0 \
    --app="$URL"
