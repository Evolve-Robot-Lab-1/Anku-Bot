/*
 * Test FL_LEG (CH3) and BR_LEG (CH6)
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 125
#define SERVOMAX 575

#define FL_LEG 3
#define BR_LEG 6

void setAngle(int ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
}

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  Serial.println("Testing FL_LEG (CH3) and BR_LEG (CH6)");

  // Start at 90
  setAngle(FL_LEG, 90);
  setAngle(BR_LEG, 90);
  delay(1000);
}

void loop() {
  Serial.println("FL_LEG 90 -> 125 (lift)");
  setAngle(FL_LEG, 125);
  delay(1000);

  Serial.println("FL_LEG 125 -> 90");
  setAngle(FL_LEG, 90);
  delay(1000);

  Serial.println("BR_LEG 90 -> 55 (lift)");
  setAngle(BR_LEG, 55);
  delay(1000);

  Serial.println("BR_LEG 55 -> 90");
  setAngle(BR_LEG, 90);
  delay(1000);

  delay(500);
}
