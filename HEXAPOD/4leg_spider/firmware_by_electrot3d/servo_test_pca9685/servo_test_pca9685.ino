/*
 * QUADRUPED ROBOT - FIRST TEST with PCA9685
 * Arduino UNO + PCA9685 Servo Driver
 *
 * WIRING:
 *   PCA9685    Arduino UNO
 *   ---------  -----------
 *   VCC   -->  5V
 *   GND   -->  GND
 *   SDA   -->  A4
 *   SCL   -->  A5
 *   V+    -->  External 5-6V power supply (for servos)
 *
 * SERVO CONNECTIONS on PCA9685:
 *   Channel 0: Front Right Hip
 *   Channel 1: Front Left Hip
 *   Channel 2: Front Right Leg
 *   Channel 3: Front Left Leg
 *   Channel 4: Back Right Hip
 *   Channel 5: Back Left Hip
 *   Channel 6: Back Right Leg
 *   Channel 7: Back Left Leg
 *
 * Install library: Adafruit PWM Servo Driver Library
 * In Arduino IDE: Sketch -> Include Library -> Manage Libraries -> Search "Adafruit PWM"
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Create PCA9685 object (default address 0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Servo pulse length limits (adjust if needed for your servos)
#define SERVOMIN  125  // Minimum pulse length (0 degrees)
#define SERVOMAX  575  // Maximum pulse length (180 degrees)
#define SERVO_FREQ 50  // 50 Hz for standard servos

// Number of servos
#define NUM_SERVOS 8

// Servo names for display
const char* servoNames[] = {
  "Front Right Hip", "Front Left Hip",
  "Front Right Leg", "Front Left Leg",
  "Back Right Hip",  "Back Left Hip",
  "Back Right Leg",  "Back Left Leg"
};

// Convert angle (0-180) to PWM pulse length
int angleToPulse(int angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

// Move a single servo to specified angle
void setServoAngle(int channel, int angle) {
  int pulse = angleToPulse(angle);
  pwm.setPWM(channel, 0, pulse);
}

// Move all servos to specified angle
void setAllServos(int angle) {
  for (int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, angle);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("=================================");
  Serial.println("QUADRUPED ROBOT - PCA9685 TEST");
  Serial.println("=================================");
  Serial.println();

  // Initialize PCA9685
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);

  delay(10);

  Serial.println("PCA9685 Initialized!");
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  c - Center all servos (90 degrees)");
  Serial.println("  t - Test each servo one by one");
  Serial.println("  s - Sweep all servos");
  Serial.println("  h - Home position (standing)");
  Serial.println("  0-7 - Move specific servo");
  Serial.println();

  // Center all servos on startup
  Serial.println("Centering all servos...");
  setAllServos(90);
  delay(1000);
  Serial.println("Ready! Send command via Serial Monitor.");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'c':
      case 'C':
        // Center all servos
        Serial.println("Centering all servos to 90 degrees...");
        setAllServos(90);
        break;

      case 't':
      case 'T':
        // Test each servo one by one
        testEachServo();
        break;

      case 's':
      case 'S':
        // Sweep test
        sweepTest();
        break;

      case 'h':
      case 'H':
        // Home/standing position
        homePosition();
        break;

      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        // Test individual servo
        testSingleServo(cmd - '0');
        break;
    }
  }
}

void testEachServo() {
  Serial.println("\n--- Testing each servo ---");

  // First center all
  setAllServos(90);
  delay(500);

  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print("Testing servo ");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(servoNames[i]);
    Serial.println(")");

    // Move to 60 degrees
    setServoAngle(i, 60);
    delay(500);

    // Move to 120 degrees
    setServoAngle(i, 120);
    delay(500);

    // Back to center
    setServoAngle(i, 90);
    delay(300);
  }

  Serial.println("Test complete!\n");
}

void testSingleServo(int servo) {
  Serial.print("\n--- Testing servo ");
  Serial.print(servo);
  Serial.print(" (");
  Serial.print(servoNames[servo]);
  Serial.println(") ---");

  for (int angle = 45; angle <= 135; angle += 15) {
    Serial.print("  Angle: ");
    Serial.println(angle);
    setServoAngle(servo, angle);
    delay(400);
  }

  setServoAngle(servo, 90);
  Serial.println("Done.\n");
}

void sweepTest() {
  Serial.println("\n--- Sweep test ---");

  // Sweep from 60 to 120 degrees
  for (int angle = 60; angle <= 120; angle += 2) {
    setAllServos(angle);
    delay(30);
  }

  for (int angle = 120; angle >= 60; angle -= 2) {
    setAllServos(angle);
    delay(30);
  }

  setAllServos(90);
  Serial.println("Sweep complete!\n");
}

void homePosition() {
  Serial.println("\n--- Home Position (Standing) ---");

  // Hip servos slightly angled, leg servos at 90
  int ap = 20;  // hip angle offset

  setServoAngle(0, 90 + ap);  // Front Right Hip
  setServoAngle(1, 90 - ap);  // Front Left Hip
  setServoAngle(2, 90);       // Front Right Leg
  setServoAngle(3, 90);       // Front Left Leg
  setServoAngle(4, 90 - ap);  // Back Right Hip
  setServoAngle(5, 90 + ap);  // Back Left Hip
  setServoAngle(6, 90);       // Back Right Leg
  setServoAngle(7, 90);       // Back Left Leg

  Serial.println("Home position set.\n");
}
