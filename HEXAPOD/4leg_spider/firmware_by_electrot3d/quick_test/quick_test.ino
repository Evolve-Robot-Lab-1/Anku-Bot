/*
 * Quick servo test - all servos sweep
 */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 125
#define SERVOMAX 575

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

  Serial.println("Testing all servos...");

  // Set all to 90
  for (int i = 0; i < 8; i++) {
    setAngle(i, 90);
  }
  delay(1000);
}

void loop() {
  // Move all servos to 60
  Serial.println("All to 60");
  for (int i = 0; i < 8; i++) {
    setAngle(i, 60);
  }
  delay(500);

  // Move all servos to 120
  Serial.println("All to 120");
  for (int i = 0; i < 8; i++) {
    setAngle(i, 120);
  }
  delay(500);
}
