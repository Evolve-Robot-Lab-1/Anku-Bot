# Tailwind Setup (PC + Client Laptop)

This repo now includes a local Tailwind CLI toolchain in `naval_web_control`.

## 1) Your PC (done once)

```bash
cd ~/Anku-Bot/Naval/naval_web_control
npm install
npm run build:css
```

## 2) Client Laptop (same setup)

```bash
cd ~/Anku-Bot/Naval/naval_web_control
npm install
npm run build:css
```

For live edits:

```bash
npm run watch:css
```

## 3) Robot update with latest HDMI commit

```bash
ssh pi@<robot-ip-or-tailscale-ip>
cd ~/naval_ws/src/naval_web_control
git pull --ff-only origin main
bash start_naval.sh
```

Disable HDMI kiosk for one run if needed:

```bash
bash start_naval.sh --no-hdmi
```
