/*
 * MiniKame PCA9685 Implementation - FIXED timing
 */

#include "MiniKame_PCA.h"

MiniKame_PCA::MiniKame_PCA() : pwm() {
}

void MiniKame_PCA::init() {
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
  home();
}

void MiniKame_PCA::setServo(int ch, int angle) {
  angle = constrain(angle + trim[ch], 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
}

void MiniKame_PCA::setTrim(int ch, int value) {
  trim[ch] = constrain(value, -30, 30);
}

int MiniKame_PCA::getTrim(int ch) {
  return trim[ch];
}

void MiniKame_PCA::setSpeed(int speed) {
  speedFactor = constrain(speed, 50, 200);
}

void MiniKame_PCA::delay_ms(int ms) {
  delay(ms * 100 / speedFactor);
}

void MiniKame_PCA::home() {
  setServo(FR_HIP, homeHip[0]);
  setServo(FL_HIP, homeHip[1]);
  setServo(BR_HIP, homeHip[2]);
  setServo(BL_HIP, homeHip[3]);
  setServo(FR_LEG, homeLeg);
  setServo(FL_LEG, homeLeg);
  setServo(BR_LEG, homeLeg);
  setServo(BL_LEG, homeLeg);
}

void MiniKame_PCA::moveServos(int time, int target[8]) {
  for (int i = 0; i < 8; i++) {
    setServo(i, target[i]);
  }
  delay(time);
}

// ==================== WALK (FIXED - matches working walk_test) ====================
void MiniKame_PCA::walk(int steps, int period, int dir) {
  int hipSwing = 25;
  int legLift = 35;

  for (int s = 0; s < steps; s++) {
    // Step 1: Lift FR and BL
    setServo(FR_LEG, homeLeg - legLift);
    setServo(BL_LEG, homeLeg + legLift);
    delay(100);

    // Swing hips (FIXED for your robot - directions swapped)
    if (dir == 1) {  // Forward
      setServo(FR_HIP, homeHip[0] - hipSwing);
      setServo(BL_HIP, homeHip[3] + hipSwing);
      setServo(FL_HIP, homeHip[1] - hipSwing);
      setServo(BR_HIP, homeHip[2] + hipSwing);
    } else {  // Backward
      setServo(FR_HIP, homeHip[0] + hipSwing);
      setServo(BL_HIP, homeHip[3] - hipSwing);
      setServo(FL_HIP, homeHip[1] + hipSwing);
      setServo(BR_HIP, homeHip[2] - hipSwing);
    }
    delay(150);

    // Put FR and BL down
    setServo(FR_LEG, homeLeg);
    setServo(BL_LEG, homeLeg);
    delay(100);

    // Step 2: Lift FL and BR (more lift for these legs)
    setServo(FL_LEG, homeLeg + legLift + 15);
    setServo(BR_LEG, homeLeg - legLift - 15);
    delay(100);

    // Swing hips (FIXED for your robot - directions swapped)
    if (dir == 1) {  // Forward
      setServo(FL_HIP, homeHip[1] + hipSwing);
      setServo(BR_HIP, homeHip[2] - hipSwing);
      setServo(FR_HIP, homeHip[0] + hipSwing);
      setServo(BL_HIP, homeHip[3] - hipSwing);
    } else {  // Backward
      setServo(FL_HIP, homeHip[1] - hipSwing);
      setServo(BR_HIP, homeHip[2] + hipSwing);
      setServo(FR_HIP, homeHip[0] - hipSwing);
      setServo(BL_HIP, homeHip[3] + hipSwing);
    }
    delay(150);

    // Put FL and BR down
    setServo(FL_LEG, homeLeg);
    setServo(BR_LEG, homeLeg);
    delay(100);

    delay(100);  // Extra delay between steps (matches walk_test)
  }
  home();
}

// ==================== RUN (FIXED) ====================
void MiniKame_PCA::run(int steps, int period, int dir) {
  int hipSwing = 20;
  int legLift = 25;

  for (int s = 0; s < steps; s++) {
    // Step 1
    setServo(FR_LEG, homeLeg - legLift);
    setServo(BL_LEG, homeLeg + legLift);

    if (dir == 1) {  // Forward - directions swapped
      setServo(FR_HIP, homeHip[0] - hipSwing);
      setServo(BL_HIP, homeHip[3] + hipSwing);
      setServo(FL_HIP, homeHip[1] - hipSwing);
      setServo(BR_HIP, homeHip[2] + hipSwing);
    } else {  // Backward
      setServo(FR_HIP, homeHip[0] + hipSwing);
      setServo(BL_HIP, homeHip[3] - hipSwing);
      setServo(FL_HIP, homeHip[1] + hipSwing);
      setServo(BR_HIP, homeHip[2] - hipSwing);
    }
    delay(100);

    setServo(FR_LEG, homeLeg);
    setServo(BL_LEG, homeLeg);
    delay(50);

    // Step 2
    setServo(FL_LEG, homeLeg + legLift + 10);
    setServo(BR_LEG, homeLeg - legLift - 10);

    if (dir == 1) {  // Forward - directions swapped
      setServo(FL_HIP, homeHip[1] + hipSwing);
      setServo(BR_HIP, homeHip[2] - hipSwing);
      setServo(FR_HIP, homeHip[0] + hipSwing);
      setServo(BL_HIP, homeHip[3] - hipSwing);
    } else {  // Backward
      setServo(FL_HIP, homeHip[1] - hipSwing);
      setServo(BR_HIP, homeHip[2] + hipSwing);
      setServo(FR_HIP, homeHip[0] - hipSwing);
      setServo(BL_HIP, homeHip[3] + hipSwing);
    }
    delay(100);

    setServo(FL_LEG, homeLeg);
    setServo(BR_LEG, homeLeg);
    delay(50);
  }
  home();
}

// ==================== TURN LEFT (FIXED) ====================
void MiniKame_PCA::turnLeft(int steps, int period) {
  int hipSwing = 25;
  int legLift = 35;

  for (int s = 0; s < steps; s++) {
    // Lift FR and BL (diagonal)
    setServo(FR_LEG, homeLeg - legLift);
    setServo(BL_LEG, homeLeg + legLift);
    delay(100);

    // Rotate: all hips same direction
    setServo(FR_HIP, homeHip[0] - hipSwing);
    setServo(BR_HIP, homeHip[2] - hipSwing);
    setServo(FL_HIP, homeHip[1] - hipSwing);
    setServo(BL_HIP, homeHip[3] - hipSwing);
    delay(150);

    // Put down
    setServo(FR_LEG, homeLeg);
    setServo(BL_LEG, homeLeg);
    delay(100);

    // Lift FL and BR
    setServo(FL_LEG, homeLeg + legLift + 15);
    setServo(BR_LEG, homeLeg - legLift - 15);
    delay(100);

    // Continue rotation
    setServo(FR_HIP, homeHip[0] + hipSwing);
    setServo(BR_HIP, homeHip[2] + hipSwing);
    setServo(FL_HIP, homeHip[1] + hipSwing);
    setServo(BL_HIP, homeHip[3] + hipSwing);
    delay(150);

    // Put down
    setServo(FL_LEG, homeLeg);
    setServo(BR_LEG, homeLeg);
    delay(100);
  }
  home();
}

// ==================== TURN RIGHT (FIXED) ====================
void MiniKame_PCA::turnRight(int steps, int period) {
  int hipSwing = 25;
  int legLift = 35;

  for (int s = 0; s < steps; s++) {
    // Lift FR and BL (diagonal)
    setServo(FR_LEG, homeLeg - legLift);
    setServo(BL_LEG, homeLeg + legLift);
    delay(100);

    // Rotate: all hips same direction
    setServo(FR_HIP, homeHip[0] + hipSwing);
    setServo(BR_HIP, homeHip[2] + hipSwing);
    setServo(FL_HIP, homeHip[1] + hipSwing);
    setServo(BL_HIP, homeHip[3] + hipSwing);
    delay(150);

    // Put down
    setServo(FR_LEG, homeLeg);
    setServo(BL_LEG, homeLeg);
    delay(100);

    // Lift FL and BR
    setServo(FL_LEG, homeLeg + legLift + 15);
    setServo(BR_LEG, homeLeg - legLift - 15);
    delay(100);

    // Continue rotation
    setServo(FR_HIP, homeHip[0] - hipSwing);
    setServo(BR_HIP, homeHip[2] - hipSwing);
    setServo(FL_HIP, homeHip[1] - hipSwing);
    setServo(BL_HIP, homeHip[3] - hipSwing);
    delay(150);

    // Put down
    setServo(FL_LEG, homeLeg);
    setServo(BR_LEG, homeLeg);
    delay(100);
  }
  home();
}

// ==================== DANCE ====================
void MiniKame_PCA::dance(int steps, int period) {
  int legSwing = 35;

  for (int s = 0; s < steps; s++) {
    // Wiggle left
    setServo(FR_LEG, homeLeg - legSwing);
    setServo(BR_LEG, homeLeg - legSwing);
    setServo(FL_LEG, homeLeg + legSwing);
    setServo(BL_LEG, homeLeg + legSwing);
    delay(300);

    // Wiggle right
    setServo(FR_LEG, homeLeg + legSwing);
    setServo(BR_LEG, homeLeg + legSwing);
    setServo(FL_LEG, homeLeg - legSwing);
    setServo(BL_LEG, homeLeg - legSwing);
    delay(300);

    // Twist hips
    setServo(FR_HIP, homeHip[0] + 15);
    setServo(BL_HIP, homeHip[3] + 15);
    setServo(FL_HIP, homeHip[1] - 15);
    setServo(BR_HIP, homeHip[2] - 15);
    delay(300);

    setServo(FR_HIP, homeHip[0] - 15);
    setServo(BL_HIP, homeHip[3] - 15);
    setServo(FL_HIP, homeHip[1] + 15);
    setServo(BR_HIP, homeHip[2] + 15);
    delay(300);
  }
  home();
}

// ==================== MOONWALK ====================
void MiniKame_PCA::moonwalk(int steps, int period) {
  int legSwing = 35;

  for (int s = 0; s < steps; s++) {
    setServo(FR_LEG, homeLeg);
    setServo(FL_LEG, homeLeg + legSwing);
    setServo(BR_LEG, homeLeg - legSwing);
    setServo(BL_LEG, homeLeg);
    delay(250);

    setServo(FR_LEG, homeLeg - legSwing);
    setServo(FL_LEG, homeLeg);
    setServo(BR_LEG, homeLeg);
    setServo(BL_LEG, homeLeg + legSwing);
    delay(250);

    setServo(FR_LEG, homeLeg);
    setServo(FL_LEG, homeLeg - legSwing);
    setServo(BR_LEG, homeLeg + legSwing);
    setServo(BL_LEG, homeLeg);
    delay(250);

    setServo(FR_LEG, homeLeg + legSwing);
    setServo(FL_LEG, homeLeg);
    setServo(BR_LEG, homeLeg);
    setServo(BL_LEG, homeLeg - legSwing);
    delay(250);
  }
  home();
}

// ==================== PUSH UP ====================
void MiniKame_PCA::pushUp(int steps, int period) {
  for (int s = 0; s < steps; s++) {
    // Down
    setServo(FR_LEG, homeLeg - 45);
    setServo(FL_LEG, homeLeg + 45);
    setServo(BR_LEG, homeLeg - 45);
    setServo(BL_LEG, homeLeg + 45);
    delay(500);

    // Up
    setServo(FR_LEG, homeLeg + 15);
    setServo(FL_LEG, homeLeg - 15);
    setServo(BR_LEG, homeLeg + 15);
    setServo(BL_LEG, homeLeg - 15);
    delay(500);
  }
  home();
}

// ==================== UP DOWN ====================
void MiniKame_PCA::upDown(int steps, int period) {
  int legMove = 25;

  for (int s = 0; s < steps; s++) {
    // Crouch
    setServo(FR_LEG, homeLeg - legMove);
    setServo(FL_LEG, homeLeg + legMove);
    setServo(BR_LEG, homeLeg - legMove);
    setServo(BL_LEG, homeLeg + legMove);
    delay(300);

    // Stand tall
    setServo(FR_LEG, homeLeg + legMove);
    setServo(FL_LEG, homeLeg - legMove);
    setServo(BR_LEG, homeLeg + legMove);
    setServo(BL_LEG, homeLeg - legMove);
    delay(300);
  }
  home();
}

// ==================== FRONT BACK ====================
void MiniKame_PCA::frontBack(int steps, int period) {
  int legMove = 25;

  for (int s = 0; s < steps; s++) {
    // Lean forward
    setServo(FR_LEG, homeLeg + legMove);
    setServo(FL_LEG, homeLeg - legMove);
    setServo(BR_LEG, homeLeg - legMove);
    setServo(BL_LEG, homeLeg + legMove);
    delay(400);

    // Lean back
    setServo(FR_LEG, homeLeg - legMove);
    setServo(FL_LEG, homeLeg + legMove);
    setServo(BR_LEG, homeLeg + legMove);
    setServo(BL_LEG, homeLeg - legMove);
    delay(400);
  }
  home();
}

// ==================== HELLO (FIXED) ====================
void MiniKame_PCA::hello() {
  // Sit back - lean on back legs
  setServo(FR_LEG, homeLeg + 30);
  setServo(FL_LEG, homeLeg - 30);
  setServo(BR_LEG, homeLeg - 30);
  setServo(BL_LEG, homeLeg + 30);
  delay(500);

  // Raise front right leg high
  setServo(FR_LEG, homeLeg - 50);
  delay(300);

  // Wave
  for (int i = 0; i < 3; i++) {
    setServo(FR_HIP, homeHip[0] - 30);
    delay(300);
    setServo(FR_HIP, homeHip[0] + 30);
    delay(300);
  }

  delay(300);
  home();
}

// ==================== JUMP ====================
void MiniKame_PCA::jump() {
  // Crouch
  setServo(FR_LEG, homeLeg - 45);
  setServo(FL_LEG, homeLeg + 45);
  setServo(BR_LEG, homeLeg - 45);
  setServo(BL_LEG, homeLeg + 45);
  delay(400);

  // Jump!
  setServo(FR_LEG, homeLeg + 45);
  setServo(FL_LEG, homeLeg - 45);
  setServo(BR_LEG, homeLeg + 45);
  setServo(BL_LEG, homeLeg - 45);
  delay(150);

  home();
  delay(300);
}
