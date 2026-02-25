/*
 * WALK TEST - Bluetooth + Serial Control
 * Quadruped robot with PCA9685
 * Bluetooth: RX=D2, TX=D3
 * Commands: F=forward, B=backward, L=left, R=right, W=hello, H=home
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <SoftwareSerial.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
SoftwareSerial BT(2, 3);  // RX=D2, TX=D3

#define SERVOMIN  125
#define SERVOMAX  575

// Channel mapping
#define FR_HIP 0
#define FL_HIP 1
#define FR_LEG 2
#define FL_LEG 3
#define BR_HIP 4
#define BL_HIP 5
#define BR_LEG 6
#define BL_LEG 7

// Home positions
int homeHip[4] = {70, 110, 110, 70};  // FR, FL, BR, BL
int homeLeg = 90;

// Movement parameters (original working values)
int hipSwing = 25;
int legLift = 35;

void setAngle(int ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
}

void homePosition() {
  setAngle(FR_HIP, homeHip[0]);
  setAngle(FL_HIP, homeHip[1]);
  setAngle(BR_HIP, homeHip[2]);
  setAngle(BL_HIP, homeHip[3]);
  setAngle(FR_LEG, homeLeg);
  setAngle(FL_LEG, homeLeg);
  setAngle(BR_LEG, homeLeg);
  setAngle(BL_LEG, homeLeg);
}

void walkForward(int steps) {
  for (int s = 0; s < steps; s++) {
    // Step 1: Lift FR and BL
    setAngle(FR_LEG, homeLeg - legLift);
    setAngle(BL_LEG, homeLeg + legLift);
    delay(100);

    setAngle(FR_HIP, homeHip[0] - hipSwing);
    setAngle(BL_HIP, homeHip[3] + hipSwing);
    setAngle(FL_HIP, homeHip[1] - hipSwing);
    setAngle(BR_HIP, homeHip[2] + hipSwing);
    delay(150);

    setAngle(FR_LEG, homeLeg);
    setAngle(BL_LEG, homeLeg);
    delay(100);

    // Step 2: Lift FL and BR (more lift)
    setAngle(FL_LEG, homeLeg + legLift + 15);
    setAngle(BR_LEG, homeLeg - legLift - 15);
    delay(100);

    setAngle(FL_HIP, homeHip[1] + hipSwing);
    setAngle(BR_HIP, homeHip[2] - hipSwing);
    setAngle(FR_HIP, homeHip[0] + hipSwing);
    setAngle(BL_HIP, homeHip[3] - hipSwing);
    delay(150);

    setAngle(FL_LEG, homeLeg);
    setAngle(BR_LEG, homeLeg);
    delay(100);

  }
  homePosition();
}

void walkBackward(int steps) {
  for (int s = 0; s < steps; s++) {
    // Step 1: Lift FR and BL
    setAngle(FR_LEG, homeLeg - legLift);
    setAngle(BL_LEG, homeLeg + legLift);
    delay(100);

    setAngle(FR_HIP, homeHip[0] + hipSwing);
    setAngle(BL_HIP, homeHip[3] - hipSwing);
    setAngle(FL_HIP, homeHip[1] + hipSwing);
    setAngle(BR_HIP, homeHip[2] - hipSwing);
    delay(150);

    setAngle(FR_LEG, homeLeg);
    setAngle(BL_LEG, homeLeg);
    delay(100);

    // Step 2: Lift FL and BR
    setAngle(FL_LEG, homeLeg + legLift);
    setAngle(BR_LEG, homeLeg - legLift);
    delay(100);

    setAngle(FL_HIP, homeHip[1] - hipSwing);
    setAngle(BR_HIP, homeHip[2] + hipSwing);
    setAngle(FR_HIP, homeHip[0] - hipSwing);
    setAngle(BL_HIP, homeHip[3] + hipSwing);
    delay(150);

    setAngle(FL_LEG, homeLeg);
    setAngle(BR_LEG, homeLeg);
    delay(100);
  }
  homePosition();
}

void turnLeft(int steps) {
  for (int s = 0; s < steps; s++) {
    // Lift FR and BL (diagonal)
    setAngle(FR_LEG, homeLeg - legLift);
    setAngle(BL_LEG, homeLeg + legLift);
    delay(100);

    // Rotate: right side forward, left side back
    setAngle(FR_HIP, homeHip[0] - hipSwing);
    setAngle(BR_HIP, homeHip[2] - hipSwing);
    setAngle(FL_HIP, homeHip[1] - hipSwing);
    setAngle(BL_HIP, homeHip[3] - hipSwing);
    delay(150);

    // Put down
    setAngle(FR_LEG, homeLeg);
    setAngle(BL_LEG, homeLeg);
    delay(100);

    // Lift FL and BR
    setAngle(FL_LEG, homeLeg + legLift);
    setAngle(BR_LEG, homeLeg - legLift);
    delay(100);

    // Continue rotation
    setAngle(FR_HIP, homeHip[0] + hipSwing);
    setAngle(BR_HIP, homeHip[2] + hipSwing);
    setAngle(FL_HIP, homeHip[1] + hipSwing);
    setAngle(BL_HIP, homeHip[3] + hipSwing);
    delay(150);

    // Put down
    setAngle(FL_LEG, homeLeg);
    setAngle(BR_LEG, homeLeg);
    delay(100);
  }
  homePosition();
}

void turnRight(int steps) {
  for (int s = 0; s < steps; s++) {
    // Lift FR and BL (diagonal)
    setAngle(FR_LEG, homeLeg - legLift);
    setAngle(BL_LEG, homeLeg + legLift);
    delay(100);

    // Rotate: left side forward, right side back
    setAngle(FR_HIP, homeHip[0] + hipSwing);
    setAngle(BR_HIP, homeHip[2] + hipSwing);
    setAngle(FL_HIP, homeHip[1] + hipSwing);
    setAngle(BL_HIP, homeHip[3] + hipSwing);
    delay(150);

    // Put down
    setAngle(FR_LEG, homeLeg);
    setAngle(BL_LEG, homeLeg);
    delay(100);

    // Lift FL and BR
    setAngle(FL_LEG, homeLeg + legLift);
    setAngle(BR_LEG, homeLeg - legLift);
    delay(100);

    // Continue rotation
    setAngle(FR_HIP, homeHip[0] - hipSwing);
    setAngle(BR_HIP, homeHip[2] - hipSwing);
    setAngle(FL_HIP, homeHip[1] - hipSwing);
    setAngle(BL_HIP, homeHip[3] - hipSwing);
    delay(150);

    // Put down
    setAngle(FL_LEG, homeLeg);
    setAngle(BR_LEG, homeLeg);
    delay(100);
  }
  homePosition();
}

void hello() {
  // Sit back - lean on back legs
  setAngle(FR_LEG, homeLeg + 30);   // Front legs up
  setAngle(FL_LEG, homeLeg - 30);
  setAngle(BR_LEG, homeLeg - 30);   // Back legs down
  setAngle(BL_LEG, homeLeg + 30);
  delay(500);

  // Raise front right leg high
  setAngle(FR_LEG, homeLeg - 50);
  delay(300);

  // Wave
  for (int i = 0; i < 3; i++) {
    setAngle(FR_HIP, homeHip[0] - 30);
    delay(300);
    setAngle(FR_HIP, homeHip[0] + 30);
    delay(300);
  }

  delay(300);
  homePosition();
}

void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  homePosition();

  Serial.println("Ready! F=forward B=back L=left R=right W=hello H=home");
}

void loop() {
  // Check Bluetooth first
  if (BT.available()) {
    char cmd = BT.read();
    runCommand(cmd);
  }

  // Check Serial
  if (Serial.available()) {
    char cmd = Serial.read();
    runCommand(cmd);
  }
}

void runCommand(char cmd) {
  switch(cmd) {
    case 'F': walkForward(1); delay(200); break;
    case 'B': walkBackward(1); delay(200); break;
    case 'L': turnLeft(1); break;
    case 'R': turnRight(1); break;
    case 'W': hello(); break;
    case 'H': homePosition(); break;
  }
}
