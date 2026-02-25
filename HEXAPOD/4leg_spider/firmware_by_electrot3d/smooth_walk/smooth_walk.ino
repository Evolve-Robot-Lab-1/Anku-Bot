/*
 * SMOOTH WALK - Direct copy of walk_test values
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  125
#define SERVOMAX  575

// Current positions
float pos[8];

void setServo(int ch, float angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map((int)angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
  pos[ch] = angle;
}

void smoothMove(float target[8], int duration) {
  float start[8];
  for (int i = 0; i < 8; i++) start[i] = pos[i];

  int steps = duration / 10;
  if (steps < 1) steps = 1;

  for (int s = 1; s <= steps; s++) {
    float t = (float)s / steps;
    t = t * t * (3 - 2 * t);

    for (int i = 0; i < 8; i++) {
      setServo(i, start[i] + (target[i] - start[i]) * t);
    }
    delay(10);
  }
}

void home() {
  // Exact walk_test home: {70, 110, 90, 90, 110, 70, 90, 90}
  float h[8] = {70, 110, 90, 90, 110, 70, 90, 90};
  smoothMove(h, 300);
}

void walkForward(int numSteps) {
  /*
   * EXACT values from walk_test.ino:
   * Step 1: FR_LEG=55, BL_LEG=125, FR_HIP=45, FL_HIP=85, BR_HIP=135, BL_HIP=95
   * Step 2: FL_LEG=140, BR_LEG=40, FR_HIP=95, FL_HIP=135, BR_HIP=85, BL_HIP=45
   */

  for (int s = 0; s < numSteps; s++) {
    // Step 1a: Lift FR + BL
    float s1a[8] = {pos[0], pos[1], 55, pos[3], pos[4], pos[5], pos[6], 125};
    smoothMove(s1a, 100);

    // Step 1b: Swing hips
    float s1b[8] = {45, 85, 55, pos[3], 135, 95, pos[6], 125};
    smoothMove(s1b, 150);

    // Step 1c: Put down FR + BL
    float s1c[8] = {45, 85, 90, pos[3], 135, 95, pos[6], 90};
    smoothMove(s1c, 100);

    // Step 2a: Lift FL + BR
    float s2a[8] = {pos[0], pos[1], pos[2], 140, pos[4], pos[5], 40, pos[7]};
    smoothMove(s2a, 100);

    // Step 2b: Swing hips
    float s2b[8] = {95, 135, pos[2], 140, 85, 45, 40, pos[7]};
    smoothMove(s2b, 150);

    // Step 2c: Put down FL + BR
    float s2c[8] = {95, 135, pos[2], 90, 85, 45, 90, pos[7]};
    smoothMove(s2c, 100);
  }
  home();
}

void walkBackward(int numSteps) {
  // Opposite hip directions from forward
  for (int s = 0; s < numSteps; s++) {
    // Step 1a: Lift FR + BL
    float s1a[8] = {pos[0], pos[1], 55, pos[3], pos[4], pos[5], pos[6], 125};
    smoothMove(s1a, 100);

    // Step 1b: Swing hips (opposite)
    float s1b[8] = {95, 135, 55, pos[3], 85, 45, pos[6], 125};
    smoothMove(s1b, 150);

    // Step 1c: Put down
    float s1c[8] = {95, 135, 90, pos[3], 85, 45, pos[6], 90};
    smoothMove(s1c, 100);

    // Step 2a: Lift FL + BR
    float s2a[8] = {pos[0], pos[1], pos[2], 140, pos[4], pos[5], 40, pos[7]};
    smoothMove(s2a, 100);

    // Step 2b: Swing hips (opposite)
    float s2b[8] = {45, 85, pos[2], 140, 135, 95, 40, pos[7]};
    smoothMove(s2b, 150);

    // Step 2c: Put down
    float s2c[8] = {45, 85, pos[2], 90, 135, 95, 90, pos[7]};
    smoothMove(s2c, 100);
  }
  home();
}

void turnLeft(int numSteps) {
  for (int s = 0; s < numSteps; s++) {
    // Lift FR + BL
    float t1[8] = {pos[0], pos[1], 55, pos[3], pos[4], pos[5], pos[6], 125};
    smoothMove(t1, 100);

    // All hips same direction (decrease)
    float t2[8] = {45, 85, 55, pos[3], 85, 45, pos[6], 125};
    smoothMove(t2, 150);

    float t3[8] = {45, 85, 90, pos[3], 85, 45, pos[6], 90};
    smoothMove(t3, 100);

    // Lift FL + BR
    float t4[8] = {pos[0], pos[1], pos[2], 140, pos[4], pos[5], 40, pos[7]};
    smoothMove(t4, 100);

    // All hips opposite (increase)
    float t5[8] = {95, 135, pos[2], 140, 135, 95, 40, pos[7]};
    smoothMove(t5, 150);

    float t6[8] = {95, 135, pos[2], 90, 135, 95, 90, pos[7]};
    smoothMove(t6, 100);
  }
  home();
}

void turnRight(int numSteps) {
  for (int s = 0; s < numSteps; s++) {
    // Lift FR + BL
    float t1[8] = {pos[0], pos[1], 55, pos[3], pos[4], pos[5], pos[6], 125};
    smoothMove(t1, 100);

    // All hips increase
    float t2[8] = {95, 135, 55, pos[3], 135, 95, pos[6], 125};
    smoothMove(t2, 150);

    float t3[8] = {95, 135, 90, pos[3], 135, 95, pos[6], 90};
    smoothMove(t3, 100);

    // Lift FL + BR
    float t4[8] = {pos[0], pos[1], pos[2], 140, pos[4], pos[5], 40, pos[7]};
    smoothMove(t4, 100);

    // All hips decrease
    float t5[8] = {45, 85, pos[2], 140, 85, 45, 40, pos[7]};
    smoothMove(t5, 150);

    float t6[8] = {45, 85, pos[2], 90, 85, 45, 90, pos[7]};
    smoothMove(t6, 100);
  }
  home();
}

void setup() {
  Serial.begin(9600);

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  for (int i = 0; i < 8; i++) pos[i] = 90;
  home();

  Serial.println(F("SMOOTH WALK v3"));
  Serial.println(F("w=fwd s=back a=left d=right h=home"));
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'w': Serial.println(F("Fwd")); walkForward(4); break;
      case 's': Serial.println(F("Back")); walkBackward(4); break;
      case 'a': Serial.println(F("Left")); turnLeft(4); break;
      case 'd': Serial.println(F("Right")); turnRight(4); break;
      case 'h': Serial.println(F("Home")); home(); break;
    }
  }
}
