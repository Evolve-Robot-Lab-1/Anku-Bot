/*
 * QUADRUPED ROBOT - SERVO CALIBRATION TOOL
 * Arduino UNO + PCA9685
 *
 * Use this to calibrate servo offsets before running the robot.
 * Each servo may need small adjustments (+/- degrees) to make
 * the robot stand symmetrically.
 *
 * WIRING: Same as test sketch (see servo_test_pca9685.ino)
 *
 * HOW TO CALIBRATE:
 * 1. Upload this sketch
 * 2. Open Serial Monitor (9600 baud)
 * 3. Select a servo (0-7)
 * 4. Adjust trim with +/- until leg is straight
 * 5. Repeat for all servos
 * 6. Save calibration (press 'w')
 * 7. Copy the trim values to your main robot code
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  125
#define SERVOMAX  575
#define SERVO_FREQ 50
#define NUM_SERVOS 8

// EEPROM storage
#define EEPROM_MAGIC 0xAB
#define EEPROM_START 0

// Servo names
const char* servoNames[] = {
  "FR Hip ", "FL Hip ",   // Front Right/Left Hip
  "FR Leg ", "FL Leg ",   // Front Right/Left Leg
  "BR Hip ", "BL Hip ",   // Back Right/Left Hip
  "BR Leg ", "BL Leg "    // Back Right/Left Leg
};

// Current servo selection
int currentServo = 0;

// Trim values (offset in degrees, can be negative)
int trim[NUM_SERVOS] = {0, 0, 0, 0, 0, 0, 0, 0};

// Base angles for home position
int homeAngle[NUM_SERVOS] = {
  110,  // 0: Front Right Hip (90 + 20)
  70,   // 1: Front Left Hip  (90 - 20)
  90,   // 2: Front Right Leg
  90,   // 3: Front Left Leg
  70,   // 4: Back Right Hip  (90 - 20)
  110,  // 5: Back Left Hip   (90 + 20)
  90,   // 6: Back Right Leg
  90    // 7: Back Left Leg
};

int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void setServo(int channel, int angle) {
  pwm.setPWM(channel, 0, angleToPulse(angle));
}

void applyHomePosition() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    setServo(i, homeAngle[i] + trim[i]);
  }
}

void applySingleServo(int servo) {
  setServo(servo, homeAngle[servo] + trim[servo]);
}

void loadTrimFromEEPROM() {
  if (EEPROM.read(EEPROM_START) == EEPROM_MAGIC) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      int8_t val = EEPROM.read(EEPROM_START + 1 + i);
      if (val >= -45 && val <= 45) {
        trim[i] = val;
      }
    }
    Serial.println("Loaded trim from EEPROM");
  } else {
    Serial.println("No saved calibration found");
  }
}

void saveTrimToEEPROM() {
  EEPROM.write(EEPROM_START, EEPROM_MAGIC);
  for (int i = 0; i < NUM_SERVOS; i++) {
    EEPROM.write(EEPROM_START + 1 + i, (int8_t)trim[i]);
  }
  Serial.println("\n*** CALIBRATION SAVED TO EEPROM ***\n");
}

void printStatus() {
  Serial.println("\n========== SERVO CALIBRATION ==========");
  Serial.println("Servo    | Base | Trim | Final");
  Serial.println("---------|------|------|------");

  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(servoNames[i]);
    Serial.print("| ");

    if (homeAngle[i] < 100) Serial.print(" ");
    Serial.print(homeAngle[i]);
    Serial.print(" | ");

    if (trim[i] >= 0) Serial.print("+");
    if (abs(trim[i]) < 10) Serial.print(" ");
    Serial.print(trim[i]);
    Serial.print(" | ");

    int final_angle = homeAngle[i] + trim[i];
    if (final_angle < 100) Serial.print(" ");
    Serial.print(final_angle);

    if (i == currentServo) {
      Serial.print("  <-- SELECTED");
    }
    Serial.println();
  }

  Serial.println("========================================");
  Serial.println("\nCommands:");
  Serial.println("  0-7    : Select servo");
  Serial.println("  + or ] : Increase trim (+1)");
  Serial.println("  - or [ : Decrease trim (-1)");
  Serial.println("  > or } : Increase trim (+5)");
  Serial.println("  < or { : Decrease trim (-5)");
  Serial.println("  z      : Zero current servo trim");
  Serial.println("  Z      : Zero ALL trims");
  Serial.println("  h      : Apply home position");
  Serial.println("  c      : Center current servo (90 deg)");
  Serial.println("  w      : Save calibration to EEPROM");
  Serial.println("  p      : Print C++ code for trim values");
  Serial.println("  ?      : Show this status\n");
}

void printCodeOutput() {
  Serial.println("\n// Copy this to your robot code:");
  Serial.println("// --------------------------------");
  Serial.print("int trim[8] = {");
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(trim[i]);
    if (i < NUM_SERVOS - 1) Serial.print(", ");
  }
  Serial.println("};");
  Serial.println("// --------------------------------\n");
}

void setup() {
  Serial.begin(9600);

  Serial.println("\n");
  Serial.println("####################################");
  Serial.println("#  QUADRUPED SERVO CALIBRATION     #");
  Serial.println("#  Arduino UNO + PCA9685           #");
  Serial.println("####################################\n");

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  // Load any saved calibration
  loadTrimFromEEPROM();

  // Apply home position
  Serial.println("Applying home position...\n");
  applyHomePosition();
  delay(500);

  printStatus();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    // Select servo 0-7
    if (cmd >= '0' && cmd <= '7') {
      currentServo = cmd - '0';
      Serial.print("\n>> Selected servo ");
      Serial.print(currentServo);
      Serial.print(" (");
      Serial.print(servoNames[currentServo]);
      Serial.println(")");

      // Wiggle the selected servo to identify it
      int base = homeAngle[currentServo] + trim[currentServo];
      setServo(currentServo, base - 10);
      delay(150);
      setServo(currentServo, base + 10);
      delay(150);
      setServo(currentServo, base);
    }

    // Increase trim
    else if (cmd == '+' || cmd == '=') {
      trim[currentServo]++;
      trim[currentServo] = constrain(trim[currentServo], -45, 45);
      applySingleServo(currentServo);
      Serial.print("Servo ");
      Serial.print(currentServo);
      Serial.print(" trim: ");
      if (trim[currentServo] >= 0) Serial.print("+");
      Serial.println(trim[currentServo]);
    }

    // Decrease trim
    else if (cmd == '-' || cmd == '_') {
      trim[currentServo]--;
      trim[currentServo] = constrain(trim[currentServo], -45, 45);
      applySingleServo(currentServo);
      Serial.print("Servo ");
      Serial.print(currentServo);
      Serial.print(" trim: ");
      if (trim[currentServo] >= 0) Serial.print("+");
      Serial.println(trim[currentServo]);
    }

    // Big increase (+5)
    else if (cmd == '>' || cmd == ']' || cmd == '}') {
      trim[currentServo] += 5;
      trim[currentServo] = constrain(trim[currentServo], -45, 45);
      applySingleServo(currentServo);
      Serial.print("Servo ");
      Serial.print(currentServo);
      Serial.print(" trim: ");
      if (trim[currentServo] >= 0) Serial.print("+");
      Serial.println(trim[currentServo]);
    }

    // Big decrease (-5)
    else if (cmd == '<' || cmd == '[' || cmd == '{') {
      trim[currentServo] -= 5;
      trim[currentServo] = constrain(trim[currentServo], -45, 45);
      applySingleServo(currentServo);
      Serial.print("Servo ");
      Serial.print(currentServo);
      Serial.print(" trim: ");
      if (trim[currentServo] >= 0) Serial.print("+");
      Serial.println(trim[currentServo]);
    }

    // Zero current servo trim
    else if (cmd == 'z') {
      trim[currentServo] = 0;
      applySingleServo(currentServo);
      Serial.print("Servo ");
      Serial.print(currentServo);
      Serial.println(" trim reset to 0");
    }

    // Zero ALL trims
    else if (cmd == 'Z') {
      for (int i = 0; i < NUM_SERVOS; i++) {
        trim[i] = 0;
      }
      applyHomePosition();
      Serial.println("ALL trims reset to 0");
    }

    // Apply home position
    else if (cmd == 'h' || cmd == 'H') {
      applyHomePosition();
      Serial.println("Home position applied");
    }

    // Center current servo at 90
    else if (cmd == 'c' || cmd == 'C') {
      setServo(currentServo, 90);
      Serial.print("Servo ");
      Serial.print(currentServo);
      Serial.println(" centered at 90 degrees (ignoring trim)");
    }

    // Save to EEPROM
    else if (cmd == 'w' || cmd == 'W') {
      saveTrimToEEPROM();
      printCodeOutput();
    }

    // Print code output
    else if (cmd == 'p' || cmd == 'P') {
      printCodeOutput();
    }

    // Show status
    else if (cmd == '?' || cmd == '/') {
      printStatus();
    }
  }
}
