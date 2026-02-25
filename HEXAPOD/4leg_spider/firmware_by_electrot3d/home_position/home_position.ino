/*
 * HOME POSITION - Robot Standing (FIXED)
 * Hip angles swapped
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  125
#define SERVOMAX  575

void setAngle(int ch, int angle) {
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
}

void setup() {
  Serial.begin(9600);

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  Serial.println("=== HOME POSITION (FIXED) ===");

  // HIPS - swapped angles for outward
  setAngle(0, 70);   // Front Right Hip (90 - 20)
  setAngle(1, 110);  // Front Left Hip  (90 + 20)
  setAngle(4, 110);  // Back Right Hip  (90 + 20)
  setAngle(5, 70);   // Back Left Hip   (90 - 20)

  // LEGS - straight down
  setAngle(2, 90);   // Front Right Leg
  setAngle(3, 90);   // Front Left Leg
  setAngle(6, 90);   // Back Right Leg
  setAngle(7, 90);   // Back Left Leg

  Serial.println("Hips: FR=70, FL=110, BR=110, BL=70");
  Serial.println("Legs: All at 90 (down)");
  Serial.println("\nRobot should be standing!");
}

void loop() {
  delay(1000);
}
