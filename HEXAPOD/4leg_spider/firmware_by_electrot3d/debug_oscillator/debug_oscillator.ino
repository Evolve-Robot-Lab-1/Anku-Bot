/*
 * Debug test - simple oscillator movement
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 125
#define SERVOMAX 575

void setServo(int ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Debug test starting...");

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  // Set home
  setServo(0, 110);  // FR Hip
  setServo(1, 70);   // FL Hip
  setServo(2, 90);   // FR Leg
  setServo(3, 90);   // FL Leg
  setServo(4, 70);   // BR Hip
  setServo(5, 110);  // BL Hip
  setServo(6, 90);   // BR Leg
  setServo(7, 90);   // BL Leg

  delay(1000);
  Serial.println("Home set. Starting oscillation test...");
  Serial.println("Press any key to start");
}

void loop() {
  if (Serial.available()) {
    Serial.read();

    Serial.println("Running simple leg oscillation...");

    // Simple oscillation - move legs up and down using sine wave
    unsigned long startTime = millis();
    int period = 1000;  // 1 second period
    int amplitude = 30;

    while (millis() - startTime < 4000) {  // Run for 4 seconds
      float t = (millis() - startTime) % period;
      float angle = amplitude * sin(2 * 3.14159 * t / period);

      // Move all legs
      setServo(2, 90 + angle);   // FR Leg
      setServo(3, 90 - angle);   // FL Leg (inverted)
      setServo(6, 90 + angle);   // BR Leg
      setServo(7, 90 - angle);   // BL Leg (inverted)

      delay(10);
    }

    // Return to home
    setServo(2, 90);
    setServo(3, 90);
    setServo(6, 90);
    setServo(7, 90);

    Serial.println("Done! Press any key to run again.");
  }
}
