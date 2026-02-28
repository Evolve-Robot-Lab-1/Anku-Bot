# Bot 1 — Arm servos on PCA CH0 + CH1: working points (detailed)

Date: 2026-10-07. Pi `192.168.1.8` (user `sanjeev`), Mega 2560 R3 on `/dev/ttyACM0`
(USB `2341:0042`), 115200 baud. Stable Pi path:
`/dev/serial/by-id/usb-Arduino__www.arduino.cc__0042_1344A47413035101E7D9-if00`.

## Hardware

- Servo 2 on PCA9685 **CH0** (first slot). Second servo on PCA **CH1**.
  Channels count 0-1-2 on the PCA9685 header — verify by counting, not by guess.
- PCA9685 I2C address `0x41`, 50 Hz (`pca.setPWMFreq(50)`), OE on Mega D7 (active low).
- Servo power: external V+ rail to the PCA terminal block (Mega USB does NOT power servos).
  Shared ground between servo supply and Mega required.
- CH0 horn was removed for free-servo tests; reattach status after that is unconfirmed —
  re-validate under load before trusting attached positions.
- Base motors forced off in this build (PWM D8–D12 low, direction D22–D29 low).
- No encoder/motor telemetry in this build (servo-only).

## Working points (saved)

Current Mega state: dual hold `HOLDING,CH0,2220,CH1,1335`.

### CH0 (servo 2)

| Name | Degrees (user scale) | Pulse (us) | PCA counts | State |
|---|---|---|---|---|
| UP | 200 | 2110 | ~432 | verified (`Q` pose) |
| MID | 210 | 2165 | ~444 | verified (`R` pose) |
| FLAT-lock (holding) | 220 | 2220 | ~455 | `SERVO_HOLDING,CH0,2220` |

- Window: 200 ↔ 220 = 110 us ≈ 20° nominal shaft travel.
- `W` sweeps 2110 → 2220 → 2110 (2.5 s/leg, cubic ease elbow) and holds 2110.
- `V` slow-sweeps 2220 → 2165 → 2220 (5 s/leg) with CH1 locked, holds 2220.
  Last `V` run: `SERVO_START,CH0,SLOW220_TO_210`, `SERVO_CYCLE_COMPLETE,CH0`,
  `SERVO_HOLDING,CH0,2220`.

### CH1 (second servo)

| Name | Degrees | Pulse (us) | State |
|---|---|---|---|
| base (holding) | 60 | 1335 | `SERVO_HOLDING,CH1,1335` |

- Reached by walking 0 → 5 → 10 → 20 → 40 → 60 from `M0` (1000 us).
- CH1 mapping/limits beyond 0–60° are unverified; horn on/off status unconfirmed.

## Angle ↔ pulse map (nominal, 5.56 us/°)

| Angle | Pulse | Notes |
|---|---|---|
| 0 | 1000 | verified free (`N0`) |
| 60 | ~1335 | verified free |
| 90 | 1500 | verified free (`N90`) |
| 180 | 2000 | verified free (`N180`) |
| 190 | ~2055 | verified free, smooth |
| 200 (UP) | 2110 | holding |
| 220 (FLAT) | 2220 | verified, smooth |
| firmware clamp | 900–2250 | `E`/`T` jog ±5 us, `gotoUs` rejects outside |

Notes:
- Servo 2 travels past nominal 180° smoothly (likely 270°-class). 190° (2055),
  200° (2110), 220° (2220) all held free; 240 was refused (past 2250 ceiling).
- Attached-arm limits differ from free-servo capability: with the old horn clocking
  the arm hit its mechanical up-stop near 1880 us (servo tried, fell back).
  Reattach the horn at mid-travel before loaded use and re-validate under load.
- CH0 direction on this linkage: lower pulse = up, higher pulse = down.

## Firmware on the Mega (which code)

- Source: `arduino/ch0_sweep/ch0_sweep.ino` (this repo), sketch 11,656 bytes.
- Pi hex: `/tmp/ch0_sweep.hex` — AVRDUDE verified (`avrdude -p atmega2560 -c wiring
  -P /dev/ttyACM0 -b 115200 -D -U flash:w:/tmp/ch0_sweep.hex:i`).
- Banner: `READY,CH0_CH1_DUAL`, PCA `0x41` FOUND.
- Replaces the 4-motor firmware on the Mega. To restore wheels/encoders, reflash
  `arduino/four_motor_test/four_motor_test.ino` (has the `uint16_t` shoulder fix + `E`/`T` jog).

### Serial commands (115200 baud, newline-terminated)

| Cmd | Action |
|---|---|
| `Q` | dual pose: CH0 200 (2110) + CH1 60 (1335). Works from `STOPPED`. |
| `C` | carry-all: CH0 220 (2220) + CH1 60 (1335) + CH11 0 (1000). One command restore. |
| `F` | **rest**: CH0 210 (2165) + CH1 60 (1335) + CH11 60 (1335). |
| CH11 0 (1000) | **pick-ready** position. |
| `G<ch>,<us>` | generic setter, e.g. `G11,1335`; keeps other holds alive. |
| `R` | dual pose: CH0 210 (2165) + CH1 60 (1335). Works from `STOPPED`. |
| `P` | dual pose: CH0 220 (2220) + CH1 60 (1335). Works from `STOPPED`. |
| `V` | slow sweep CH0 220 ↔ 210 (5 s/leg), CH1 locked. Requires CH1 holding (run `P` first). Ends holding 2220. |
| `W` | window sweep CH0 200 ↔ 220 (2.5 s/leg), ends holding 2110. Keeps CH1 hold alive. |
| `A` | legacy sweep CH0 2000 → 1917 → 2000, holds 2000. Keeps CH1 hold alive. |
| `B` | window sweep CH1 200 ↔ 220, ends holding CH1 2110. Keeps CH0 hold alive. |
| `Z` | CH0 goto 2000 hold (keeps CH1). `X` = CH1 goto 2000 hold (keeps CH0). |
| `N0` / `N90` / `N180` | CH0 goto 1000 / 1500 / 2000 (keeps CH1). |
| `M0` / `M90` / `M180` | CH1 goto 1000 / 1500 / 2000 (keeps CH0). |
| `E` / `T` | CH0 −5 / +5 us jog from hold (900–2250 clamp). |
| `Y` / `U` | CH1 −5 / +5 us jog from hold (900–2250 clamp). |
| `STATUS` | `RUNNING`, or `HOLDING,CH0,<us>,CH1,<us>` (0 = released), never `STOPPED` alone anymore. |
| `STOP` / `S` | release all outputs (servos go slack — support the arm first). |

Caution: opening the serial port resets the Mega (holds lost → released). The one-shot
poses (`Q`/`R`/`P`) exist so a full dual pose is one command after the reset blip.
Keep one session open per move sequence; never send `STOP` while the arm needs holding.

## Procedures

Build + flash (from `hackathon_bot/` on the PC):

```bash
arduino-cli compile --fqbn arduino:avr:mega:cpu=atmega2560 --export-binaries arduino/ch0_sweep
scp arduino/ch0_sweep/build/arduino.avr.mega/ch0_sweep.ino.hex sanjeev@192.168.1.8:/tmp/ch0_sweep.hex
ssh sanjeev@192.168.1.8 "avrdude -p atmega2560 -c wiring -P /dev/ttyACM0 -b 115200 -D -U flash:w:/tmp/ch0_sweep.hex:i"
```

Flashing resets the Mega and releases all outputs for ~10 s — support the arm first.

Drive to a pose (example 220, from any state):

```python
import serial, time
s = serial.Serial('/dev/ttyACM0', 115200, timeout=2)
time.sleep(0.5)
# opening resets Mega; wait for banner:
while True:
    if s.readline().decode(errors='replace').startswith('READY,'): break
s.write(b'P\n')  # or Q / R
```

## Troubleshooting seen in this session

1. **Empty-channel driving:** servo 2 was on CH0/CH1 while commands went to CH2.
   Fix: count PCA slots from 0; confirm plug channel before driving.
2. **`uint8_t` truncation slam** (in old `four_motor_test`): `startShoulder(uint8_t)`
   turned 995 → 227, driving CH2 to ~230 us. Fixed to `uint16_t`; released via `STOP`.
3. **Horn clocking:** 990–1100 us range sat fully in the arm-up region; fixed electrically
   by moving zero to 2000 (down). Proper fix is re-clocking the horn at mid-travel.
4. **Single-channel holds drop the arm:** old builds disabled all outputs during a sweep.
   This build rewrites both latched holds (`writeHolds()`) so the idle joint stays powered.
5. **Mechanical stops:** up-stop hit ~1880 (old clocking, attached); servo stalled then fell
   back. Back off ~20 us and treat as limit. Free-servo range is wider than attached range.
6. **Silent non-motion with good serial replies:** check servo V+ rail, plug seating, and that
   the commanded channel actually has a servo on it.

## Full source (as flashed)

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Clean CH0-only servo sweep for Bot 1.
// Servo 2 is plugged on PCA CH0. No other servo is driven.
// PCA9685 @ 0x41, OE on Mega D7 (active low).
// Sweep: 1000 -> 1083 us (~15 deg) -> 1000, 2.5 s per leg, cubic ease.
// Holds the start pulse after completion so the joint does not sag.
// STOP disables all PCA outputs (joint goes slack, keep arm supported).

Adafruit_PWMServoDriver pca(0x41);
const uint8_t PCA_OE_PIN = 7;
const uint8_t CH0 = 0;
const uint8_t CH1 = 1;
const uint16_t START_US = 2000;
const uint16_t END_US = 1917;
const uint16_t LEG_MS = 2500;
const uint8_t STEP_MS = 20;

bool pcaFound = false;
bool sweeping = false;
bool holding[2] = {false, false};
uint16_t holdUs[2] = {START_US, START_US};
uint8_t swCh = 0;
uint16_t swStartUs = START_US;
uint16_t swEndUs = END_US;
uint16_t swLegMs = LEG_MS;
uint32_t startedAt = 0;
uint32_t stepAt = 0;
char cmd[24];
uint8_t cmdLen = 0;
bool overflow = false;

static uint16_t usToCounts(uint16_t us) {
  return uint32_t(us) * 4096UL / 20000UL;
}

void disableOutputs() {
  digitalWrite(PCA_OE_PIN, HIGH);
  sweeping = false;
  holding[0] = holding[1] = false;
  if (pcaFound) {
    for (uint8_t ch = 0; ch < 16; ++ch) pca.setPWM(ch, 0, 4096);
  }
}

// Keep the other channel's hold alive while driving one channel: rewrite
// both latched pulses, then enable outputs. Full-off would drop the arm.
void writeHolds() {
  if (pcaFound) {
    if (holding[0]) pca.setPWM(CH0, 0, usToCounts(holdUs[0]));
    if (holding[1]) pca.setPWM(CH1, 0, usToCounts(holdUs[1]));
  }
  digitalWrite(PCA_OE_PIN, LOW);
}

void gotoUs(uint8_t ch, uint16_t target) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 1) { Serial.println(F("ERROR,CHANNEL")); return; }
  if (target < 900 || target > 2250) { Serial.println(F("ERROR,RANGE_900_2250")); return; }
  sweeping = false;
  holding[ch] = true;
  holdUs[ch] = target;
  writeHolds();
  Serial.print(ch == 0 ? F("SERVO_HOLDING,CH0,") : F("SERVO_HOLDING,CH1,"));
  Serial.println(holdUs[ch]);
}

void gotoZero() {
  gotoUs(CH0, START_US);
}

// One-shot dual poses (work from STOPPED, no prior hold needed).
void gotoPose(uint16_t ch0us, uint16_t ch1us) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch0us < 900 || ch0us > 2250 || ch1us < 900 || ch1us > 2250) {
    Serial.println(F("ERROR,RANGE_900_2250")); return;
  }
  sweeping = false;
  holding[0] = holding[1] = true;
  holdUs[0] = ch0us;
  holdUs[1] = ch1us;
  writeHolds();
  Serial.print(F("POSE,CH0,")); Serial.print(holdUs[0]);
  Serial.print(F(",CH1,")); Serial.println(holdUs[1]);
}

void jog(uint8_t ch, int8_t stepUs) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 1) { Serial.println(F("ERROR,CHANNEL")); return; }
  if (!holding[ch]) { Serial.println(F("ERROR,NOT_HOLDING")); return; }
  int16_t target = int16_t(holdUs[ch]) + stepUs;
  if (target < 900 || target > 2250) {
    Serial.print(F("ERROR,LIMIT,")); Serial.println(holdUs[ch]);
    return;
  }
  holdUs[ch] = uint16_t(target);
  writeHolds();
  Serial.print(ch == 0 ? F("SERVO_HOLDING,CH0,") : F("SERVO_HOLDING,CH1,"));
  Serial.println(holdUs[ch]);
}
void startSweep(uint8_t ch) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 1) { Serial.println(F("ERROR,CHANNEL")); return; }
  sweeping = true;
  holding[ch] = false;
  swCh = ch;
  swStartUs = START_US;
  swEndUs = END_US;
  swLegMs = LEG_MS;
  startedAt = millis();
  stepAt = millis() - STEP_MS;
  Serial.print(F("SERVO_START,CH")); Serial.print(swCh); Serial.print(F(","));
  Serial.print(swStartUs); Serial.print(F("_TO_"));
  Serial.print(swEndUs); Serial.println(F("_TO_START_US"));
}

// Working window sweep: 200 deg (2110 us) <-> 220 deg (2220 us), holds low end.
void startWindowSweep(uint8_t ch) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 1) { Serial.println(F("ERROR,CHANNEL")); return; }
  sweeping = true;
  holding[ch] = false;
  swCh = ch;
  swStartUs = 2110;
  swEndUs = 2220;
  swLegMs = LEG_MS;
  startedAt = millis();
  stepAt = millis() - STEP_MS;
  Serial.print(F("SERVO_START,CH")); Serial.print(swCh);
  Serial.println(F(",WIN200_TO_220"));
}

// Slow sweep: CH0 220 flat-lock (2220) <-> 210 rise (2165), 5 s/leg.
// Requires CH1 already holding (stays locked at its pulse throughout).
void startSlowSweep() {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (!holding[1]) { Serial.println(F("ERROR,CH1_NOT_HOLDING")); return; }
  sweeping = true;
  holding[0] = false;
  swCh = 0;
  swStartUs = 2220;
  swEndUs = 2165;
  swLegMs = 5000;
  startedAt = millis();
  stepAt = millis() - STEP_MS;
  Serial.println(F("SERVO_START,CH0,SLOW220_TO_210"));
}

void serviceSweep() {
  if (!sweeping) return;
  if (uint32_t(millis() - stepAt) < STEP_MS) return;
  stepAt = millis();
  uint32_t elapsed = uint32_t(millis() - startedAt);
  bool returning = elapsed >= swLegMs;
  uint32_t legElapsed = returning ? elapsed - swLegMs : elapsed;
  if (legElapsed > swLegMs) legElapsed = swLegMs;
  uint32_t t = legElapsed * 1024UL / swLegMs;
  uint32_t eased = t * t * (3072UL - 2UL * t) / (1024UL * 1024UL);
  uint16_t fromUs = returning ? swEndUs : swStartUs;
  uint16_t toUs = returning ? swStartUs : swEndUs;
  int32_t q = int32_t(fromUs) * 1024L + (int32_t(toUs) - fromUs) * int32_t(eased);
  pca.setPWM(swCh, 0, usToCounts(uint16_t(q / 1024L)));
  uint8_t other = swCh ^ 1;
  if (holding[other]) pca.setPWM(other, 0, usToCounts(holdUs[other]));
  digitalWrite(PCA_OE_PIN, LOW);
  if (elapsed >= 2UL * swLegMs) {
    sweeping = false;
    holding[swCh] = true;
    holdUs[swCh] = swStartUs;
    writeHolds();
    Serial.print(F("SERVO_CYCLE_COMPLETE,CH")); Serial.println(swCh);
    Serial.print(F("SERVO_HOLDING,CH")); Serial.print(swCh); Serial.print(F(","));
    Serial.println(holdUs[swCh]);
  }
}

void execCmd() {
  cmd[cmdLen] = ' ';
  if (overflow) { disableOutputs(); Serial.println(F("ERROR,COMMAND_TOO_LONG")); }
  else if (!strcmp(cmd, "A")) startSweep(0);
  else if (!strcmp(cmd, "W")) startWindowSweep(0);
  else if (!strcmp(cmd, "Z")) gotoZero();
  else if (!strcmp(cmd, "E")) jog(0, -5);
  else if (!strcmp(cmd, "T")) jog(0, 5);
  else if (!strcmp(cmd, "N0")) gotoUs(0, 1000);
  else if (!strcmp(cmd, "N90")) gotoUs(0, 1500);
  else if (!strcmp(cmd, "N180")) gotoUs(0, 2000);
  else if (!strcmp(cmd, "B")) startWindowSweep(1);
  else if (!strcmp(cmd, "X")) gotoUs(1, 2000);
  else if (!strcmp(cmd, "Y")) jog(1, -5);
  else if (!strcmp(cmd, "U")) jog(1, 5);
  else if (!strcmp(cmd, "M0")) gotoUs(1, 1000);
  else if (!strcmp(cmd, "M90")) gotoUs(1, 1500);
  else if (!strcmp(cmd, "M180")) gotoUs(1, 2000);
  else if (!strcmp(cmd, "Q")) gotoPose(2110, 1335);
  else if (!strcmp(cmd, "P")) gotoPose(2220, 1335);
  else if (!strcmp(cmd, "R")) gotoPose(2165, 1335);
  else if (!strcmp(cmd, "V")) startSlowSweep();
  else if (!strcmp(cmd, "STOP") || !strcmp(cmd, "S")) { disableOutputs(); Serial.println(F("STOPPED")); }
  else if (!strcmp(cmd, "STATUS")) {
    if (sweeping) Serial.println(F("RUNNING"));
    else {
      Serial.print(F("HOLDING,CH0,")); Serial.print(holding[0] ? holdUs[0] : 0);
      Serial.print(F(",CH1,")); Serial.println(holding[1] ? holdUs[1] : 0);
    }
  }
  else if (cmdLen) { Serial.println(F("ERROR,UNKNOWN_COMMAND")); }
  cmdLen = 0; overflow = false;
}

void setup() {
  digitalWrite(PCA_OE_PIN, HIGH);
  pinMode(PCA_OE_PIN, OUTPUT);
  // Keep base motors unpowered in this servo-only build.
  const uint8_t pwmPins[] = {8, 9, 10, 11, 12};
  for (uint8_t p : pwmPins) { digitalWrite(p, LOW); pinMode(p, OUTPUT); analogWrite(p, 0); }
  for (uint8_t p = 22; p <= 29; ++p) { digitalWrite(p, LOW); pinMode(p, OUTPUT); }
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(25000UL, true);
  bool found = false;
  for (uint8_t a = 0x08; a <= 0x77; ++a) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("I2C,FOUND,0x")); Serial.println(a, HEX);
      if (a == 0x41) found = true;
    }
  }
  if (found) {
    Serial.println(F("PCA9685,FOUND,0x41"));
    pca.begin();
    pca.setPWMFreq(50);
    pcaFound = true;
    disableOutputs();
  } else {
    Serial.println(F("PCA9685,NOT_FOUND,EXPECTED_0x41"));
  }
  Serial.println(F("READY,CH0_CH1_DUAL"));
  Serial.println(F("CH0: W win-sweep Z zero N0/N90/N180 E/T jog | CH1: B win-sweep X zero M0/M90/M180 Y/U jog | Q pose200 P pose220 R pose210 | V slow220-210 CH1-locked | STOP STATUS"));
}

void loop() {
  serviceSweep();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '
') execCmd();
    else if (c != '
') {
      if (cmdLen < sizeof(cmd) - 1) cmd[cmdLen++] = c;
      else overflow = true;
    }
  }
}
```

## Bugs fixed this session (in `four_motor_test`, kept for reference)

- `startShoulder(uint8_t target)` truncated 995 → 227 / 1000 → 232, driving CH2 to ~230 us extreme-low. Fixed to `uint16_t`. Saved as `four_motor_test.ino.pre_u8fix.bak`.
- Added CH2 relative jog `E`/`T` there too, but that firmware is superseded on the Mega by this CH0 build.

## Session history (detail)

1. **CH2 confusion:** servo 2 was physically on CH0 (later CH1), CH2 empty — early CH2
   sweeps reported serial completion with no matching motion.
2. **Truncation slam:** `C` reported `SERVO_START,CH2,995_TO_227` — `startShoulder(uint8_t)`
   cut 995→227. Released the 227 us stall with `STOP`; fixed signature to `uint16_t`
   (`four_motor_test.ino.pre_u8fix.bak`), reflashed, verified.
3. **Horn clocking:** slack rest = down, but every 995–1083 hold = up. Moved zero
   electrically to 2000 (down). Noted proper fix = re-clock horn at mid-travel.
4. **Free-servo verify (horn off):** N0/N60/N90/N180 + 190/200/220 smooth; 240 refused.
   Attached up-stop seen ~1880 (stall + fallback) under old clocking.
5. **Window locked:** base = 220 (2220), up = 200 (2110); `W` verified, holding 2110.
6. **CH1 second servo:** walked 0→5→10→20→40→60, base = 1335. Dual firmware
   (`CH0_CH1_DUAL`) keeps both holds alive; one-shot poses `Q`/`R`/`P` added.
7. **Slow sweep:** `V` (220↔210, 5 s/leg, CH1 locked) verified, holding 2220+1335.

## CH1 second servo (added later same session)

- Second servo on PCA **CH1**. Dual-channel firmware (`READY,CH0_CH1_DUAL`, 14,262 B): holds on both channels together; sweeps keep the other channel's hold alive.
- **CH1 base = 60° (1335 us), holding.** Walked 0 → 5 → 10 → 20 → 40 → 60 to get there.
- Dual hold verified: `HOLDING,CH0,2220,CH1,1335` (CH0 220 flat + CH1 60 base together).
- One-shot poses (work from `STOPPED`, no chains): `Q` = 200 (2110+1335), `R` = 210 (2165+1335), `P` = 220 (2220+1335). Firmware 14,262 B.
- `V` = slow sweep CH0 220 flat-lock (2220) ↔ 210 rise (2165), 5 s/leg, CH1 stays locked (requires CH1 holding; run `P` first). Verified: start, cycle complete, holding 2220.
- CH1 commands: `B` window sweep, `X` zero (2000), `M0/M90/M180`, `Y/U` jog ∓5 us. Same 900–2250 clamps.

## Next steps

- Reattach horn at mid-travel with servo at 90° (1500) or window mid, then re-validate 200 ↔ 220 under load.
- Restore `four_motor_test` firmware when wheel/encoder tests resume.
