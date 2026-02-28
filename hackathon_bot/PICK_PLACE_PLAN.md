# Pick-and-place plan — cam + gripper + mecanum base

Date: 2026-10-08. Status: agreed, implementation deferred until mecanum floor tests pass.

## Verified baseline (night of 2026-10-07/08)

- **Arm servos (PCA9685 @ 0x41, 50 Hz, OE D7):** CH0 shoulder, CH1 base locked 60°
  (1335 us), CH11 suction arm. Rest `F` = 210/60/60 (2165/1335/1335).
  Pick `C` = 220/60/pick-ready-0 (2220/1335/1000). Poses `Q` (200), `R` (210),
  `P` (220); slow sweep `V` (220↔210, 5 s/leg, CH1 locked); suction `O`
  (CH6 1500 + CH7 2000), release `L` (CH6 2000 + CH7 1500); setter `G<ch>,<us>`.
  Firmware: `arduino/ch0_sweep/ch0_sweep.ino` (~14.3 kB). Full detail in
  `CH0_SERVO_POINTS.md`.
- **Gripper:** suction grip confirmed on plate; release vents. Flat placement needs
  press optimization (past-220 vs dwell, TBD).
- **Vision:** Logitech C270 at `/dev/video0`, preview `http://192.168.1.8:8000/`
  (restart with `--host 0.0.0.0` — default bind address is stale).
  Plate rim recipe: `tools/detect_circles.py --min-radius 100 --max-radius 260
  --threshold 60` (script still needs copying to the Pi). No pixel→workspace
  calibration yet. Plain disk has no orientation feature.
- **Base:** `arduino/four_motor_test` bench-validated (PWM 200, 1 s pulses;
  firmware `F` = physical reverse on M1–M3, forward on M4). Floor driving untested.
  NOT on the Mega currently (servo firmware loaded, motors forced off).

## Agreed 7-step pick/drop (validated manually)

1. `C` pick pose (220 + 60 + pick-ready 0).
2. `O` suction on (grip confirmed).
3. `R` lift to 210, suction held, plate carried.
4. Base/drive adjust (pending floor tests).
5. Drop: CH1 staged to 220, then back to 60, CH0 to 220.
6. `L` release, pose held.
7. `F` rest (210 + 60 + 60).

Rules: serial-open resets the Mega (holds lost) — re-send a one-shot pose first;
never `STOP` while the arm carries load; CH1 stays 60 unless a step says otherwise.

## Phase 1 — Mechanical baseline (manual, no code)

1. Reattach CH0 horn at mid-travel if still off; tighten all horn screws.
2. Re-validate 200↔220 window + CH1 60 + CH11 range **under load**.
3. Settle flat-press: +10–30 us past 220 vs 2–3 s dwell; record what seals reliably.

## Phase 2 — Perception calibration

4. Mount the camera rigidly (bumped twice during bench work).
5. Pixel→workspace calibration: plate at 4+ known floor points, record pixel
   centers, fit mapping.
6. Orientation: add a marker (ink dot / tape) or accept center-only.
7. Copy `detect_circles.py` to the Pi; verify the tuned recipe there.

## Phase 3 — Unified firmware (key build, after floor tests)

8. Merge wheel drive into the servo build (or servo-hold into the motor build) so
   wheels + servo holds + suction hold run **simultaneously**. Neither current
   build does all three — pick-and-carry is impossible without this.
9. Keep one-shot poses, suction hold (not timed), short timed wheel pulses first,
   encoder odometry second.

## Phase 4 — Sequenced autonomy

10. Script the 7-step sequence with cam confirmation per step before chaining.
11. Vision-guided base trim: snapshot → disk offset from frame center → short
    mecanum jog → repeat until centered → pick.
12. Full loop: detect → align → pick → carry → drive → place → release → verify.

## Phase 5 — Hardening

13. E-stop policy (decide: suction holds or releases on STOP).
14. Auto re-pose after serial reset in the driver script.
15. Combined current-draw test (suction + 3 servos + 4 motors).

## Open decisions

- Orientation marker vs center-only.
- Merge direction (wheels-into-servo vs servo-into-motor).
- Order: mechanics first vs firmware first.
