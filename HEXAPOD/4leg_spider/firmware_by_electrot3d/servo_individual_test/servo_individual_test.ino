/*
 * SERVO INDIVIDUAL TEST - Interactive Testing Tool
 * Test each servo one at a time, find ranges, record data
 *
 * Use Serial Monitor at 9600 baud
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  125
#define SERVOMAX  575
#define SERVO_FREQ 50

// Current test state
int currentServo = 0;
int currentAngle = 90;

// Servo info
const char* servoNames[] = {
  "CH0: Front Right HIP",
  "CH1: Front Left HIP",
  "CH2: Front Right LEG",
  "CH3: Front Left LEG",
  "CH4: Back Right HIP",
  "CH5: Back Left HIP",
  "CH6: Back Right LEG",
  "CH7: Back Left LEG"
};

// Test data storage (you'll record these)
int recordedCenter[8] = {90, 90, 90, 90, 90, 90, 90, 90};
int recordedMin[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int recordedMax[8] = {180, 180, 180, 180, 180, 180, 180, 180};

void setAngle(int ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
}

void printHelp() {
  Serial.println(F("\n========== SERVO TEST COMMANDS =========="));
  Serial.println(F("SERVO SELECT:"));
  Serial.println(F("  0-7    : Select servo channel"));
  Serial.println(F(""));
  Serial.println(F("ANGLE CONTROL:"));
  Serial.println(F("  +      : Increase angle by 5"));
  Serial.println(F("  -      : Decrease angle by 5"));
  Serial.println(F("  ]      : Increase angle by 1"));
  Serial.println(F("  [      : Decrease angle by 1"));
  Serial.println(F("  c      : Center (90 degrees)"));
  Serial.println(F("  m      : Min (45 degrees)"));
  Serial.println(F("  x      : Max (135 degrees)"));
  Serial.println(F(""));
  Serial.println(F("QUICK ANGLES:"));
  Serial.println(F("  a      : Set to 45"));
  Serial.println(F("  s      : Set to 60"));
  Serial.println(F("  d      : Set to 75"));
  Serial.println(F("  f      : Set to 90"));
  Serial.println(F("  g      : Set to 105"));
  Serial.println(F("  h      : Set to 120"));
  Serial.println(F("  j      : Set to 135"));
  Serial.println(F(""));
  Serial.println(F("RECORD:"));
  Serial.println(F("  r      : Record current as CENTER"));
  Serial.println(F("  p      : Print all recorded data"));
  Serial.println(F(""));
  Serial.println(F("TEST:"));
  Serial.println(F("  t      : Test current servo (sweep)"));
  Serial.println(F("  w      : Wiggle test (quick back-forth)"));
  Serial.println(F("  n      : Next servo"));
  Serial.println(F("  ?      : Show this help"));
  Serial.println(F("==========================================\n"));
}

void printStatus() {
  Serial.println(F("------------------------------------------"));
  Serial.print(F("SELECTED: "));
  Serial.println(servoNames[currentServo]);
  Serial.print(F("ANGLE:    "));
  Serial.print(currentAngle);
  Serial.println(F(" degrees"));
  Serial.print(F("RECORDED CENTER: "));
  Serial.println(recordedCenter[currentServo]);
  Serial.println(F("------------------------------------------"));
}

void setup() {
  Serial.begin(9600);

  Serial.println(F("\n"));
  Serial.println(F("############################################"));
  Serial.println(F("#   SPIDER ROBOT - SERVO INDIVIDUAL TEST   #"));
  Serial.println(F("############################################"));
  Serial.println(F(""));

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  Serial.println(F("PCA9685 Ready!"));
  Serial.println(F(""));
  Serial.println(F("SERVO LAYOUT:"));
  Serial.println(F(""));
  Serial.println(F("        FRONT"));
  Serial.println(F("   [1]---+---[0]    (HIP servos)"));
  Serial.println(F("   [3]   |   [2]    (LEG servos)"));
  Serial.println(F("         |"));
  Serial.println(F("   [5]---+---[4]    (HIP servos)"));
  Serial.println(F("   [7]   |   [6]    (LEG servos)"));
  Serial.println(F("        BACK"));
  Serial.println(F(""));

  // Start all at 90
  for (int i = 0; i < 8; i++) {
    setAngle(i, 90);
  }
  delay(500);

  printHelp();
  printStatus();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    // Ignore newlines
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      // Select servo
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        currentServo = cmd - '0';
        currentAngle = 90;
        setAngle(currentServo, currentAngle);
        Serial.print(F("\n>> Selected: "));
        Serial.println(servoNames[currentServo]);
        printStatus();
        break;

      // Angle adjust
      case '+':
      case '=':
        currentAngle = constrain(currentAngle + 5, 0, 180);
        setAngle(currentServo, currentAngle);
        Serial.print(F("Angle: "));
        Serial.println(currentAngle);
        break;

      case '-':
        currentAngle = constrain(currentAngle - 5, 0, 180);
        setAngle(currentServo, currentAngle);
        Serial.print(F("Angle: "));
        Serial.println(currentAngle);
        break;

      case ']':
        currentAngle = constrain(currentAngle + 1, 0, 180);
        setAngle(currentServo, currentAngle);
        Serial.print(F("Angle: "));
        Serial.println(currentAngle);
        break;

      case '[':
        currentAngle = constrain(currentAngle - 1, 0, 180);
        setAngle(currentServo, currentAngle);
        Serial.print(F("Angle: "));
        Serial.println(currentAngle);
        break;

      // Quick angles
      case 'a': currentAngle = 45; setAngle(currentServo, currentAngle); Serial.println(F("Angle: 45")); break;
      case 's': currentAngle = 60; setAngle(currentServo, currentAngle); Serial.println(F("Angle: 60")); break;
      case 'd': currentAngle = 75; setAngle(currentServo, currentAngle); Serial.println(F("Angle: 75")); break;
      case 'f': currentAngle = 90; setAngle(currentServo, currentAngle); Serial.println(F("Angle: 90")); break;
      case 'g': currentAngle = 105; setAngle(currentServo, currentAngle); Serial.println(F("Angle: 105")); break;
      case 'h': currentAngle = 120; setAngle(currentServo, currentAngle); Serial.println(F("Angle: 120")); break;
      case 'j': currentAngle = 135; setAngle(currentServo, currentAngle); Serial.println(F("Angle: 135")); break;

      case 'c':
        currentAngle = 90;
        setAngle(currentServo, currentAngle);
        Serial.println(F("Centered at 90"));
        break;

      case 'm':
        currentAngle = 45;
        setAngle(currentServo, currentAngle);
        Serial.println(F("Min at 45"));
        break;

      case 'x':
        currentAngle = 135;
        setAngle(currentServo, currentAngle);
        Serial.println(F("Max at 135"));
        break;

      // Record center
      case 'r':
        recordedCenter[currentServo] = currentAngle;
        Serial.print(F("** RECORDED CENTER for "));
        Serial.print(servoNames[currentServo]);
        Serial.print(F(": "));
        Serial.println(currentAngle);
        break;

      // Print all data
      case 'p':
        Serial.println(F("\n======= RECORDED SERVO DATA ======="));
        Serial.println(F("CH  | Name              | Center"));
        Serial.println(F("----|-------------------|-------"));
        for (int i = 0; i < 8; i++) {
          Serial.print(i);
          Serial.print(F("   | "));
          Serial.print(servoNames[i]);
          // Pad
          for (int p = strlen(servoNames[i]); p < 18; p++) Serial.print(" ");
          Serial.print(F("| "));
          Serial.println(recordedCenter[i]);
        }
        Serial.println(F("==================================="));
        Serial.println(F("\nCopy for code:"));
        Serial.print(F("int homeAngles[8] = {"));
        for (int i = 0; i < 8; i++) {
          Serial.print(recordedCenter[i]);
          if (i < 7) Serial.print(F(", "));
        }
        Serial.println(F("};"));
        break;

      // Test sweep
      case 't':
        Serial.print(F("Testing "));
        Serial.println(servoNames[currentServo]);
        for (int a = 90; a >= 45; a -= 5) {
          setAngle(currentServo, a);
          Serial.print(a); Serial.print(F(" "));
          delay(200);
        }
        for (int a = 45; a <= 135; a += 5) {
          setAngle(currentServo, a);
          Serial.print(a); Serial.print(F(" "));
          delay(200);
        }
        for (int a = 135; a >= 90; a -= 5) {
          setAngle(currentServo, a);
          Serial.print(a); Serial.print(F(" "));
          delay(200);
        }
        Serial.println(F("\nDone"));
        currentAngle = 90;
        break;

      // Wiggle
      case 'w':
        Serial.println(F("Wiggle test..."));
        for (int i = 0; i < 3; i++) {
          setAngle(currentServo, currentAngle - 20);
          delay(200);
          setAngle(currentServo, currentAngle + 20);
          delay(200);
        }
        setAngle(currentServo, currentAngle);
        Serial.println(F("Done"));
        break;

      // Next servo
      case 'n':
        currentServo = (currentServo + 1) % 8;
        currentAngle = 90;
        setAngle(currentServo, currentAngle);
        Serial.print(F("\n>> Next: "));
        Serial.println(servoNames[currentServo]);
        printStatus();
        break;

      // Help
      case '?':
        printHelp();
        break;
    }
  }
}
