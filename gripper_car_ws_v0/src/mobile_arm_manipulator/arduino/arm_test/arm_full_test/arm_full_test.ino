/*
 * 5-DOF Arm Full Test Sketch
 *
 * Hardware:
 * - Arduino Uno/Nano
 * - PCA9685 16-Channel PWM Servo Driver (I2C Address: 0x40)
 * - 5x Servo Motors on channels 0, 1, 2, 3, 4
 *
 * Servo Mapping:
 * - Ch 0: Shoulder
 * - Ch 1: Elbow
 * - Ch 2: Wrist
 * - Ch 3: Base
 * - Ch 4: Gripper
 *
 * Wiring:
 * - PCA9685 VCC → 5V (use external 5-6V power for servos!)
 * - PCA9685 GND → GND (connect Arduino GND to external power GND)
 * - PCA9685 SDA → A4
 * - PCA9685 SCL → A5
 *
 * Controls via Serial Monitor (115200 baud):
 * - Press '1'-'5' : Select servo
 * - Press 'w'/'W' : Move selected servo forward (+5 degrees)
 * - Press 's'/'S' : Move selected servo backward (-5 degrees)
 * - Press 'q'/'Q' : Large step forward (+20 degrees)
 * - Press 'a'/'A' : Large step backward (-20 degrees)
 * - Press 'h'     : Move selected servo to home (90 degrees)
 * - Press 'H'     : Move ALL servos to home position
 * - Press 't'     : Test sweep on selected servo (0->180->0)
 * - Press 'T'     : Test sweep on ALL servos sequentially
 * - Press 'p'     : Print current positions of all servos
 * - Press '?'     : Show help menu
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 setup
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Servo channel mapping (6 channels for testing)
#define NUM_SERVOS 6
const uint8_t SERVO_CHANNELS[NUM_SERVOS] = {0, 1, 2, 3, 4, 5};
const char* SERVO_NAMES[NUM_SERVOS] = {
  "Ch0",
  "Ch1",
  "Ch2",
  "Ch3",
  "Ch4",
  "Ch5"
};

// Servo calibration (pulse values for PCA9685)
struct ServoCal {
  uint16_t minPulse;
  uint16_t maxPulse;
};

ServoCal servoCal[NUM_SERVOS] = {
  {150, 600},  // Ch 0
  {160, 610},  // Ch 1
  {150, 590},  // Ch 2
  {155, 600},  // Ch 3
  {200, 560},  // Ch 4
  {200, 560}   // Ch 5
};

// Current servo positions (in degrees, 0-180)
int servoAngles[NUM_SERVOS] = {90, 90, 90, 90, 90, 90};

// Currently selected servo (0-4)
int selectedServo = 0;

// Movement parameters
#define SMALL_STEP 5
#define LARGE_STEP 20
#define SWEEP_DELAY 30

void setup() {
  Serial.begin(115200);

  // Initialize I2C
  Wire.begin();

  // Initialize PCA9685
  pwm.begin();
  pwm.setPWMFreq(50);  // 50Hz for servos

  delay(100);

  // Initialize all servos to home position (90 degrees)
  Serial.println(F("\n============================================"));
  Serial.println(F("    5-DOF ARM FULL TEST SKETCH"));
  Serial.println(F("============================================"));
  Serial.println(F("\nInitializing servos to home position (90°)..."));

  for (int i = 0; i < NUM_SERVOS; i++) {
    servoAngles[i] = 90;
    moveServo(i, 90);
    Serial.print(F("  Servo "));
    Serial.print(i + 1);
    Serial.print(F(" (Ch "));
    Serial.print(SERVO_CHANNELS[i]);
    Serial.println(F("): 90°"));
    delay(200);
  }

  Serial.println(F("\nAll servos initialized!"));
  printHelp();
  printStatus();
}

void loop() {
  if (Serial.available() > 0) {
    char key = Serial.read();

    // Clear buffer
    while (Serial.available() > 0) {
      Serial.read();
    }

    processKey(key);
  }
}

void processKey(char key) {
  switch (key) {
    // Servo selection (1-6)
    case '1': case '2': case '3': case '4': case '5': case '6':
      selectedServo = key - '1';
      Serial.print(F("\n>> Selected: Servo "));
      Serial.print(selectedServo + 1);
      Serial.print(F(" - "));
      Serial.print(SERVO_NAMES[selectedServo]);
      Serial.print(F(" (Ch "));
      Serial.print(SERVO_CHANNELS[selectedServo]);
      Serial.print(F(") @ "));
      Serial.print(servoAngles[selectedServo]);
      Serial.println(F("°"));
      break;

    // Small step forward
    case 'w': case 'W':
      moveServoRelative(selectedServo, SMALL_STEP);
      break;

    // Small step backward
    case 's': case 'S':
      moveServoRelative(selectedServo, -SMALL_STEP);
      break;

    // Large step forward
    case 'q': case 'Q':
      moveServoRelative(selectedServo, LARGE_STEP);
      break;

    // Large step backward
    case 'a': case 'A':
      moveServoRelative(selectedServo, -LARGE_STEP);
      break;

    // Home selected servo
    case 'h':
      Serial.print(F("Homing Servo "));
      Serial.println(selectedServo + 1);
      moveServo(selectedServo, 90);
      break;

    // Home ALL servos
    case 'H':
      Serial.println(F("\nHoming ALL servos to 90°..."));
      for (int i = 0; i < NUM_SERVOS; i++) {
        moveServo(i, 90);
        delay(100);
      }
      Serial.println(F("All servos homed!"));
      break;

    // Test sweep on selected servo
    case 't':
      testSweepSingle(selectedServo);
      break;

    // Test sweep on ALL servos
    case 'T':
      testSweepAll();
      break;

    // Print status
    case 'p': case 'P':
      printStatus();
      break;

    // Help
    case '?':
      printHelp();
      break;

    // Set to 0 degrees
    case '0':
      Serial.print(F("Moving Servo "));
      Serial.print(selectedServo + 1);
      Serial.println(F(" to 0°"));
      moveServo(selectedServo, 0);
      break;

    // Set to 180 degrees
    case '9':
      Serial.print(F("Moving Servo "));
      Serial.print(selectedServo + 1);
      Serial.println(F(" to 180°"));
      moveServo(selectedServo, 180);
      break;

    default:
      // Ignore other keys
      break;
  }
}

void moveServoRelative(int servoIdx, int delta) {
  int newAngle = servoAngles[servoIdx] + delta;
  newAngle = constrain(newAngle, 0, 180);
  moveServo(servoIdx, newAngle);

  Serial.print(F("Servo "));
  Serial.print(servoIdx + 1);
  Serial.print(F(": "));
  Serial.print(servoAngles[servoIdx]);
  Serial.println(F("°"));
}

void moveServo(int servoIdx, int angle) {
  if (servoIdx < 0 || servoIdx >= NUM_SERVOS) return;

  angle = constrain(angle, 0, 180);
  servoAngles[servoIdx] = angle;

  // Convert angle to pulse value using calibration
  uint16_t pulse = map(angle, 0, 180, servoCal[servoIdx].minPulse, servoCal[servoIdx].maxPulse);

  // Set PWM
  pwm.setPWM(SERVO_CHANNELS[servoIdx], 0, pulse);
}

void testSweepSingle(int servoIdx) {
  Serial.print(F("\n--- Testing Servo "));
  Serial.print(servoIdx + 1);
  Serial.println(F(" Sweep ---"));

  // Sweep 0 -> 180
  Serial.println(F("Sweeping 0° -> 180°..."));
  for (int angle = 0; angle <= 180; angle += 5) {
    moveServo(servoIdx, angle);
    if (angle % 30 == 0) {
      Serial.print(angle);
      Serial.print(F("° "));
    }
    delay(SWEEP_DELAY);
  }
  Serial.println();

  delay(500);

  // Sweep 180 -> 0
  Serial.println(F("Sweeping 180° -> 0°..."));
  for (int angle = 180; angle >= 0; angle -= 5) {
    moveServo(servoIdx, angle);
    if (angle % 30 == 0) {
      Serial.print(angle);
      Serial.print(F("° "));
    }
    delay(SWEEP_DELAY);
  }
  Serial.println();

  // Return to 90
  moveServo(servoIdx, 90);
  Serial.println(F("Returned to 90°. Sweep complete!"));
}

void testSweepAll() {
  Serial.println(F("\n=== TESTING ALL SERVOS SEQUENTIALLY ==="));

  for (int i = 0; i < NUM_SERVOS; i++) {
    testSweepSingle(i);
    delay(500);
  }

  Serial.println(F("\n=== ALL SERVO TESTS COMPLETE ==="));
}

void printStatus() {
  Serial.println(F("\n-------- SERVO STATUS --------"));
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(F("  "));
    if (i == selectedServo) Serial.print(F(">> "));
    else Serial.print(F("   "));

    Serial.print(i + 1);
    Serial.print(F(". "));
    Serial.print(SERVO_NAMES[i]);
    Serial.print(F(" (Ch "));
    Serial.print(SERVO_CHANNELS[i]);
    Serial.print(F("): "));
    Serial.print(servoAngles[i]);
    Serial.println(F("°"));
  }
  Serial.println(F("------------------------------"));
}

void printHelp() {
  Serial.println(F("\n========== CONTROLS =========="));
  Serial.println(F("  1-6  : Select channel"));
  Serial.println(F("  w/s  : Move +/- 5°"));
  Serial.println(F("  q/a  : Move +/- 20°"));
  Serial.println(F("  0    : Move to 0°"));
  Serial.println(F("  9    : Move to 180°"));
  Serial.println(F("  h    : Home selected (90°)"));
  Serial.println(F("  H    : Home ALL (90°)"));
  Serial.println(F("  t    : Sweep test (selected)"));
  Serial.println(F("  T    : Sweep test (ALL)"));
  Serial.println(F("  p    : Print status"));
  Serial.println(F("  ?    : Show this help"));
  Serial.println(F("==============================\n"));
}
