# WiFi & Remote Access Setup

## SSH Access

### Command:
```bash
ssh pi@192.168.1.27
```

### From Different Devices:

| Device | Method |
|--------|--------|
| Linux/Mac | Terminal: `ssh pi@192.168.1.27` |
| Windows | PowerShell or [PuTTY](https://putty.org) |
| Android | Install "Termux" app |
| iPhone | Install "Terminus" app |

---

## Adding New WiFi Network

### While Connected to RPi:
```bash
ssh pi@192.168.1.27
sudo nmcli dev wifi connect "WIFI_NAME" password "WIFI_PASSWORD"
```

### List Saved Networks:
```bash
nmcli connection show
```

### Remove a Network:
```bash
sudo nmcli connection delete "WIFI_NAME"
```

---

## Recommended: Setup Phone Hotspot

This allows you to connect to robot anywhere!

```bash
ssh pi@192.168.1.27
sudo nmcli dev wifi connect "YourPhoneHotspot" password "hotspotpassword"
```

### Usage Anywhere:
1. Turn ON phone hotspot
2. Power ON RPi
3. Wait ~40 seconds
4. Open browser on phone: `http://<rpi-ip>:5000`

### Find RPi IP on Hotspot:
- Check phone hotspot settings for connected devices
- Or use: `ping raspberrypi.local`

---

## Headless WiFi Setup (No Display/SSH)

If you can't SSH, use SD card method:

1. Remove SD card from RPi
2. Insert in another PC
3. Open `boot` partition
4. Create file: `wpa_supplicant.conf`

```
country=IN
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="WIFI_NAME"
    psk="WIFI_PASSWORD"
    key_mgmt=WPA-PSK
}
```

5. Insert SD card back in RPi
6. Boot - RPi auto-connects to that WiFi

---

## Ethernet Fallback

If WiFi fails:
1. Connect RPi to router via ethernet cable
2. Find IP: `ping raspberrypi.local` or check router admin page
3. SSH in: `ssh pi@<ip-address>`
4. Fix WiFi settings

---

## Quick Reference

| Task | Command |
|------|---------|
| SSH to RPi | `ssh pi@192.168.1.27` |
| Add WiFi | `sudo nmcli dev wifi connect "NAME" password "PASS"` |
| List WiFi | `nmcli dev wifi list` |
| Show connections | `nmcli connection show` |
| Delete connection | `sudo nmcli connection delete "NAME"` |
| Check IP | `hostname -I` |
| Restart networking | `sudo systemctl restart NetworkManager` |

---

## Current Network Setup

| Setting | Value |
|---------|-------|
| RPi IP | 192.168.1.27 |
| Web Panel | http://192.168.1.27:5000 |
| Video Stream | http://192.168.1.27:8080 |
| SSH Port | 22 |

---

*Document created: January 9, 2026*
