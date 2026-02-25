/*
 * CH4 Verify - Sweep 45 to 135 (hip movement)
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

  setAngle(0, 90);
  setAngle(1, 90);
  setAngle(2, 90);
  setAngle(3, 90);
  Serial.println("CH4 Sweep: 90 -> 45 -> 90 -> 135");
}

void loop() {
  setAngle(4, 90);
  Serial.println("90 = SIDEWAYS");
  delay(1500);

  setAngle(4, 45);
  Serial.println("45 = FORWARD");
  delay(1500);

  setAngle(4, 90);
  Serial.println("90 = SIDEWAYS");
  delay(1500);

  setAngle(4, 135);
  Serial.println("135 = BACKWARD");
  delay(1500);
}
