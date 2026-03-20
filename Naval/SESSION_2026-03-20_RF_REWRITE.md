# Session Update - 2026-03-20

## Scope

Full code review and rewrite of the FlySky RF receiver firmware in `Mecanum_Encoder.ino`. The original RF code (written by GPT Codex on March 17) was reviewed, root causes for the erratic receiver behavior were identified, and all issues were fixed.

## Problems Found In March 17 Code

### Critical
1. **Polling-based PWM reading missed edges** — `sampleRfInput()` used `digitalRead()` polling in the main loop. Serial prints (5-10ms each), PID updates, and calibration routines caused the loop to miss rising/falling edges of RC PWM signals (~1-2ms pulses at 50Hz). This was the most likely root cause of erratic CH4/CH5 values observed on March 17.
2. **Pin 13 (built-in LED) used for CH2** — the LED and series resistor on D13 attenuate the input signal, causing unreliable throttle readings.

### High
3. **No pull-up on RF pins** — floating pins when receiver disconnected could produce random noise registering as valid pulses.
4. **No glitch filtering** — single noise spikes in the valid PWM range (900-2100μs) were accepted immediately.
5. **Shared timestamp across all channels** — `rfLastUpdateMs` updated on any channel activity, masking per-channel failures.

### Medium
6. **ROS stop blocked during RF control** — Pi had no way to emergency-stop the robot when RF was armed.
7. **Strafe unreachable when throttle active** — `abs(throttle) >= abs(strafe)` with discrete ±1 values meant throttle always won ties.
8. **Dead code** — `startMove()` function defined but never called.

## What Was Fixed

### 1. Pin-Change Interrupts (PCINT)
- Replaced `sampleRfInput()` polling with two hardware ISRs:
  - `ISR(PCINT0_vect)` — Port B for CH1 (D12/PB6) and CH2 (D11/PB5)
  - `ISR(PCINT2_vect)` — Port K for CH3 (A8/PK0), CH4 (A9/PK1), CH5 (A10/PK2)
- ISRs capture pulse rise/fall timestamps using `micros()`, compute width on falling edge
- Pulse widths validated in ISR (900-2100μs range check)
- No external library needed — uses raw AVR PCINT registers

### 2. Pin Reassignment
| Channel | Old Pin | New Pin | Reason |
|---------|---------|---------|--------|
| CH1 | D12 | D12 | No change (already PCINT-capable) |
| CH2 | D13 | D11 | D13 has built-in LED |
| CH3 | D46 | A8 | D46 (Port L) has no PCINT support |
| CH4 | D47 | A9 | D47 (Port L) has no PCINT support |
| CH5 | D48 | A10 | D48 (Port L) has no PCINT support |

### 3. INPUT_PULLUP on All RF Pins
- When receiver is disconnected, pins stay HIGH (no edges, no false triggers)

### 4. Glitch Filter
- New `readRfChannels()` copies ISR data with interrupts disabled
- Requires 2 consecutive raw readings within 200μs of each other before accepting
- Single noise spikes are rejected; normal stick movement passes easily

### 5. Per-Channel Timestamps
- `rfChanLastMs[5]` tracks last valid reading per channel
- `isRfSignalFresh()` now requires CH5 to be fresh AND at least one drive channel fresh
- Individual channel failures are no longer masked

### 6. ROS Emergency Stop Override
- `S` command now always works, even when RF owns drive
- Forces RF disarm (`rfArmed = false`, `rfSeenDisarm = false`)
- RF must go through full arming sequence again after ROS emergency stop

### 7. Strafe/Throttle Priority Fix
- When both throttle and strafe sticks are active simultaneously:
  - Raw pulse deviation from center (1500μs) is compared
  - Whichever axis deviates more from center wins
  - Strafe is now reachable even when throttle is slightly off-center

### 8. Dead Code Removed
- `startMove()` function deleted (was never called, duplicated `applyMotionCommand()`)

## What Was NOT Changed
- Motor drive code (driveMotor, stopAll)
- Encoder ISRs and tick counting
- PID controller (Kp, Ki, Kd, intervals)
- Calibration system (2-phase: 1s ramp + 1s measure)
- Per-motor calibration values (motorCal[])
- Camera servo control (T/G commands)
- Serial protocol (F/B/L/R/W/U/S, 1/2/3, P, C, CAL, M-test)
- Baud rate (115200)
- RF arming logic (CH5 disarm-first gate, centered sticks, 500ms hold)
- RF source arbitration (SRC_NONE / SRC_ROS / SRC_RF)

## Files Modified
- `Naval/Mecanum_Encoder/Mecanum_Encoder.ino` — firmware rewrite (498 insertions, 53 deletions)
- `Naval/FLYSKY_CT6B_FS_R6B_SETUP.md` — updated pin table and notes
- `Naval/SESSION_2026-03-17_RF_UPDATE.md` — updated wiring section with new pins

## Git
- Commit: `b45a04c`
- Branch: `main`
- Pushed to: `origin/main`

## Receiver Rewiring Required

**BEFORE uploading new firmware**, the physical receiver wiring must be changed:

```
Receiver VCC  →  Mega 5V
Receiver GND  →  Mega GND
CH1 signal    →  D12  (unchanged)
CH2 signal    →  D11  (was D13)
CH3 signal    →  A8   (was D46)
CH4 signal    →  A9   (was D47)
CH5 signal    →  A10  (was D48)
```

## Next Steps (When Resuming)

1. **Rewire receiver** to new pins (D11, A8, A9, A10)
2. Keep wheels off ground
3. Upload firmware from Pi:
   ```
   /home/pi/bin/arduino-cli compile --fqbn arduino:avr:mega Naval/Mecanum_Encoder
   /home/pi/bin/arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega Naval/Mecanum_Encoder
   ```
4. Open serial monitor and verify startup banner shows new pin map
5. Test one channel at a time in isolation:
   - Connect only CH1, move right stick left/right → expect pulse ~1000-2000
   - Connect only CH2, move left stick up/down → expect pulse ~1000-2000
   - Connect only CH5, flip switch → expect clean <1300 / >1700
6. If isolated channels read correctly, connect all and test full arming sequence
7. Verify existing web/joystick control still works with `S` command
