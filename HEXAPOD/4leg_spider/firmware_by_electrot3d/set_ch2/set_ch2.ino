/*
 * CH2 Verify - Sweep 45 to 135 (leg movement)
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

  setAngle(0, 90);  // CH0 hip
  setAngle(1, 90);  // CH1 hip
  Serial.println("CH2 Sweep: 90 -> 45 -> 90 -> 135");
}

void loop() {
  setAngle(2, 90);
  Serial.println("90 = DOWN (straight)");
  delay(1500);

  setAngle(2, 45);
  Serial.println("45 = UP/FORWARD");
  delay(1500);

  setAngle(2, 90);
  Serial.println("90 = DOWN (straight)");
  delay(1500);

  setAngle(2, 135);
  Serial.println("135 = UP/BACKWARD");
  delay(1500);
}
