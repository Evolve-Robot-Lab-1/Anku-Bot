/*
 * CH0 Verify - Sweep 45 to 135 (walking range)
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

  Serial.println("CH0 Verify: 45 <-> 90 <-> 135");
}

void loop() {
  // 90 = sideways
  setAngle(0, 90);
  Serial.println("90 = SIDEWAYS");
  delay(1500);

  // 45 = forward
  setAngle(0, 45);
  Serial.println("45 = FORWARD");
  delay(1500);

  // 90 = sideways
  setAngle(0, 90);
  Serial.println("90 = SIDEWAYS");
  delay(1500);

  // 135 = backward (home area)
  setAngle(0, 135);
  Serial.println("135 = BACKWARD");
  delay(1500);
}
