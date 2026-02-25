/*
 * SERVO CENTERING - RUN THIS FIRST!
 *
 * Use this BEFORE attaching legs to the robot.
 * This sets all servos to 90 degrees (center position).
 *
 * PROCESS:
 * 1. Connect servos to PCA9685 (don't attach legs yet!)
 * 2. Upload this sketch
 * 3. All servos move to 90 degrees
 * 4. NOW attach the leg horns in correct position
 * 5. Then run calibration sketch for fine-tuning
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  125
#define SERVOMAX  575
#define SERVO_FREQ 50

void setup() {
  Serial.begin(9600);

  Serial.println("\n================================");
  Serial.println("  SERVO CENTERING TOOL");
  Serial.println("  All servos -> 90 degrees");
  Serial.println("================================\n");

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  // Center all 8 servos to 90 degrees
  Serial.println("Centering all servos to 90 degrees...\n");

  for (int i = 0; i < 8; i++) {
    int pulse = map(90, 0, 180, SERVOMIN, SERVOMAX);
    pwm.setPWM(i, 0, pulse);

    Serial.print("Servo ");
    Serial.print(i);
    Serial.println(" -> 90 degrees");
    delay(100);
  }

  Serial.println("\n================================");
  Serial.println("  ALL SERVOS AT 90 DEGREES!");
  Serial.println("  NOW ATTACH THE LEGS");
  Serial.println("================================");
  Serial.println("\nKeep power ON while attaching legs.");
  Serial.println("See diagram below for leg positions:\n");

  Serial.println("        FRONT OF ROBOT");
  Serial.println("       _______________");
  Serial.println("      /   |     |   \\");
  Serial.println("     /    |     |    \\");
  Serial.println("    LEG   |     |   LEG");
  Serial.println("     \\    |_____|    /");
  Serial.println("      \\   |     |   /");
  Serial.println("       \\  |     |  /");
  Serial.println("      LEG |     | LEG");
  Serial.println("          |_____|");
  Serial.println("        BACK OF ROBOT\n");

  Serial.println("HIP servos: Legs point OUTWARD at 45 degrees");
  Serial.println("LEG servos: Legs point DOWNWARD (vertical)\n");
}

void loop() {
  // Keep servos powered - don't do anything else
  delay(1000);
}
