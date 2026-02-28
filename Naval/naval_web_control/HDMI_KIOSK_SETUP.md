# HDMI Dual-Camera TV View (Non-breaking Add-on)

This adds local HDMI output on Raspberry Pi while keeping current Ethernet web/joystick workflow unchanged.

## Remote headless rollout (client site via Tailscale)

Run these from your laptop while connected to Tailscale:

```bash
ssh pi@<tailscale-ip>
cd ~/naval_ws/src/naval_web_control
git pull
```

Install required desktop + browser packages:

```bash
sudo apt update
sudo apt install -y xorg openbox lightdm chromium-browser curl
```

Force HDMI output even if TV boots late:

```bash
sudo bash -c 'cat >> /boot/firmware/config.txt <<EOF
hdmi_force_hotplug=1
hdmi_group=1
hdmi_mode=16
EOF'
```

Enable auto-login desktop session:

```bash
sudo tee /etc/lightdm/lightdm.conf >/dev/null <<'EOF'
[Seat:*]
autologin-user=pi
autologin-user-timeout=0
user-session=openbox
EOF
```

Autostart HDMI kiosk:

```bash
mkdir -p ~/.config/openbox
tee ~/.config/openbox/autostart >/dev/null <<'EOF'
~/naval_ws/src/naval_web_control/scripts/naval_hdmi_kiosk.sh &
EOF
chmod +x ~/.config/openbox/autostart
chmod +x ~/naval_ws/src/naval_web_control/scripts/naval_hdmi_kiosk.sh
```

Reboot:

```bash
sudo reboot
```

After reboot: connect Pi HDMI to TV and launch robot normally. TV will show dual camera feeds from `/tv`.

## What was added

- Dual-camera TV page: `http://127.0.0.1:8090/tv`
- Kiosk launcher script: `scripts/naval_hdmi_kiosk.sh`
- Optional startup hook in `start_naval.sh` (auto-skip when no display session)
- User service template: `systemd/naval-hdmi-kiosk.service`

## One-time setup on Raspberry Pi

1. Install a browser and basic GUI session (if not already present):

```bash
sudo apt update
sudo apt install -y chromium-browser xorg openbox
```

2. Copy and enable kiosk user service:

```bash
mkdir -p ~/.config/systemd/user
cp ~/naval_ws/src/naval_web_control/systemd/naval-hdmi-kiosk.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable naval-hdmi-kiosk.service
systemctl --user start naval-hdmi-kiosk.service
```

3. Keep your existing launch flow unchanged:

```bash
bash ~/naval_ws/src/naval_web_control/start_naval.sh
```

When HDMI TV is connected and display session is available, TV shows both feeds:
- Left: `/frame/left`
- Right: `/frame/right`

## Notes

- To disable HDMI kiosk for a run:

```bash
bash start_naval.sh --no-hdmi
```

- Ethernet control panel and joystick workflow continue as before on:
  - `http://<rpi-ip>:5000`
