# Hackathon bot

This is a separate base-only workspace for the four-motor hackathon robot. The medical delivery robot remains in `../medical_robot/`. Arm work stays separate until the PCA9685 board, servo wiring, power, and safe joint limits are verified on the bench.

Latest hardware session: [SESSION_NOTES.md](SESSION_NOTES.md). On 2026-10-04, Pi SSH, USB camera, D500 raw LiDAR data, vacuum gripper, and CH0/CH1 bench sweeps were checked. Arm outputs were disabled at session end; wheel testing remains pending.

## Code in this folder

- `arduino/medical_base_reference/medical_delivery_base.ino` is an unchanged copy of the medical robot's two-channel Arduino firmware. It is a reference, **not** four-motor firmware.
- `arduino/four_wheel_base/four_wheel_base.ino` is a copy of the four-motor controller from `../gripper_car_ws_v1 (copy)/arduino/mobile_base_controller/mobile_base_controller.ino`. It has four L298N channels and encoder inputs for front-left, front-right, back-left, and back-right. It has not been validated on this hackathon robot.
- `tools/` contains copies of the medical robot's Pi Ethernet and camera utilities. The scripts still assume the original interface names and addresses; update those for the hackathon setup before use.

## Four-motor wiring assumed by the copied controller

| Wheel | PWM | Direction | Encoder A/B |
| --- | ---: | --- | --- |
| Front left | D12 | D22/D23 | D18/D31 |
| Front right | D11 | D24/D25 | D19/D33 |
| Back left | D10 | D26/D27 | D2/D35 |
| Back right | D9 | D28/D29 | D3/D37 |

The controller uses 115200 baud and stops after 2 seconds without a command. Confirm wiring, motor direction, encoder direction, and the physical E-stop before uploading or allowing wheel motion. Test with wheels isolated first.

## Arm test before integration

The prior workspace's `ARM_TESTING.md` records a PCA9685 arm test, but its channel mapping conflicts with the current `mobile_manipulator_unified_obstacle.ino` sketch and `test_arm_servos.py`. The arm firmware also uses a different serial protocol from the medical base firmware. Identify the actual PCA9685 host, I2C address, servo channel order, external servo power, and safe neutral angles before commanding any joint. Keep the base motors unpowered during the first arm test.

No firmware was uploaded and no arm or wheel movement was tested during the initial folder creation. Subsequent arm tests and firmware uploads are recorded in the session notes above; wheel movement remains untested here.
