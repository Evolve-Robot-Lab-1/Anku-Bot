/*
 * CH7 Verify - Sweep 45 to 135 (leg movement)
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

  for (int i = 0; i < 7; i++) {
    setAngle(i, 90);
  }
  Serial.println("CH7 Sweep: 90 -> 45 -> 90 -> 135");
}

void loop() {
  setAngle(7, 90);
  Serial.println("90 = DOWN (straight)");
  delay(1500);

  setAngle(7, 45);
  Serial.println("45 = UP/one side");
  delay(1500);

  setAngle(7, 90);
  Serial.println("90 = DOWN (straight)");
  delay(1500);

  setAngle(7, 135);
  Serial.println("135 = UP/other side");
  delay(1500);
}
