/*
 * TEST CHANNEL 0 ONLY
 * Sweeps servo on CH0 back and forth continuously
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
  Serial.println("\n=== CH0 SERVO TEST ===");
  Serial.println("Sweeping CH0: 45 -> 135 -> 45...\n");

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  // Start at center
  setAngle(0, 90);
  Serial.println("CH0 -> 90 degrees");
  delay(1000);
}

void loop() {
  // Sweep 45 to 135
  for (int a = 45; a <= 135; a += 5) {
    setAngle(0, a);
    Serial.print("CH0 -> ");
    Serial.println(a);
    delay(100);
  }

  delay(500);

  // Sweep 135 to 45
  for (int a = 135; a >= 45; a -= 5) {
    setAngle(0, a);
    Serial.print("CH0 -> ");
    Serial.println(a);
    delay(100);
  }

  delay(500);
}
