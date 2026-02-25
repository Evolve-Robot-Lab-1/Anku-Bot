/*
 * MOTOR + ARM TEST - Arduino Mega 2560
 * Motors (4x L298N) + Arm (PCA9685 6 servos)
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERIAL_BAUD 115200

// Motor pins
#define M_FL_IN1 22
#define M_FL_IN2 23
#define M_FL_EN 12

#define M_FR_IN1 24
#define M_FR_IN2 25
#define M_FR_EN 11

#define M_BL_IN1 26
#define M_BL_IN2 27
#define M_BL_EN 10

#define M_BR_IN1 28
#define M_BR_IN2 29
#define M_BR_EN 9

const int motor_in1[4] = { M_FL_IN1, M_FR_IN1, M_BL_IN1, M_BR_IN1 };
const int motor_in2[4] = { M_FL_IN2, M_FR_IN2, M_BL_IN2, M_BR_IN2 };
const int motor_en[4]  = { M_FL_EN, M_FR_EN, M_BL_EN, M_BR_EN };
const char* motor_names[4] = { "FL", "FR", "BL", "BR" };

// PCA9685
Adafruit_PWMServoDriver pca(0x40);
bool pca_found = false;

// Servo channels (per wiring guide)
const uint8_t SERVO_CH[6] = {0, 1, 6, 3, 4, 15};
const char* servo_names[6] = { "BASE", "SHOULDER", "ELBOW", "WRIST", "ROLL", "GRIPPER" };
float servo_angle[6] = {90, 90, 90, 90, 90, 90};

const uint16_t SERVO_MIN_US = 600;
const uint16_t SERVO_MAX_US = 2400;

String cmd_buffer = "";

// I2C scan
void i2cScan() {
  Serial.println("I2C Scan...");
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  Found: 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) Serial.println("  No I2C devices found!");
  else { Serial.print("  Total: "); Serial.println(found); }
}

void goServoAngle(uint8_t ch, float angle) {
  angle = constrain(angle, 0, 180);
  float us = SERVO_MIN_US + (angle / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US);
  pca.writeMicroseconds(ch, (uint16_t)us);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println(">>>NEW_FW_V2<<<");

  // Setup motors
  for (int i = 0; i < 4; i++) {
    pinMode(motor_in1[i], OUTPUT);
    pinMode(motor_in2[i], OUTPUT);
    pinMode(motor_en[i], OUTPUT);
    digitalWrite(motor_in1[i], LOW);
    digitalWrite(motor_in2[i], LOW);
    analogWrite(motor_en[i], 0);
  }

  // Setup I2C + PCA9685
  Wire.begin();
  i2cScan();

  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) {
    pca_found = true;
    pca.begin();
    pca.setPWMFreq(50);
    delay(500);
    Serial.println("PCA9685 OK");
  } else {
    Serial.println("PCA9685 NOT FOUND");
  }

  Serial.println("================================");
  Serial.println("MOTOR + ARM TEST - Mega 2560");
  Serial.println("================================");
  Serial.println("Commands:");
  Serial.println("  MOT,<0-3>,<-255 to 255>");
  Serial.println("  ARM,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>");
  Serial.println("  SERVO,<0-5>,<angle>");
  Serial.println("  STOP");
  Serial.println("  TEST");
  Serial.println("  STATUS");
  Serial.println("================================");
  Serial.println("READY");
}

void setMotor(int idx, int pwm) {
  if (idx < 0 || idx > 3) return;
  pwm = constrain(pwm, -255, 255);
  int mag = abs(pwm);

  if (pwm > 0) {
    digitalWrite(motor_in1[idx], HIGH);
    digitalWrite(motor_in2[idx], LOW);
  } else if (pwm < 0) {
    digitalWrite(motor_in1[idx], LOW);
    digitalWrite(motor_in2[idx], HIGH);
  } else {
    digitalWrite(motor_in1[idx], LOW);
    digitalWrite(motor_in2[idx], LOW);
  }
  analogWrite(motor_en[idx], mag);

  Serial.print("OK,MOT,");
  Serial.print(motor_names[idx]);
  Serial.print(",");
  Serial.println(pwm);
}

void stopAll() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(motor_in1[i], LOW);
    digitalWrite(motor_in2[i], LOW);
    analogWrite(motor_en[i], 0);
  }
  Serial.println("OK,STOP");
}

void processCommand(String cmd) {
  cmd.trim();

  if (cmd.startsWith("MOT,")) {
    int i1 = cmd.indexOf(',');
    int i2 = cmd.indexOf(',', i1 + 1);
    if (i2 > i1) {
      int idx = cmd.substring(i1 + 1, i2).toInt();
      int pwm = cmd.substring(i2 + 1).toInt();
      setMotor(idx, pwm);
    }
  }
  else if (cmd.startsWith("ARM,")) {
    if (!pca_found) { Serial.println("ERROR,PCA9685 not found"); return; }
    String t = cmd.substring(4);
    float vals[6];
    int index = 0, last = 0;
    for (int i = 0; i <= (int)t.length(); i++) {
      if (i == (int)t.length() || t.charAt(i) == ',') {
        vals[index++] = t.substring(last, i).toFloat();
        last = i + 1;
        if (index >= 6) break;
      }
    }
    if (index == 6) {
      for (int i = 0; i < 6; i++) {
        servo_angle[i] = constrain(vals[i], 0, 180);
        goServoAngle(SERVO_CH[i], servo_angle[i]);
      }
      Serial.println("OK,ARM");
    } else {
      Serial.println("ERROR,ARM needs 6 values");
    }
  }
  else if (cmd.startsWith("SERVO,")) {
    if (!pca_found) { Serial.println("ERROR,PCA9685 not found"); return; }
    int c1 = cmd.indexOf(',');
    int c2 = cmd.indexOf(',', c1 + 1);
    int idx = cmd.substring(c1 + 1, c2).toInt();
    float ang = cmd.substring(c2 + 1).toFloat();
    if (idx >= 0 && idx < 6) {
      servo_angle[idx] = constrain(ang, 0, 180);
      goServoAngle(SERVO_CH[idx], servo_angle[idx]);
      Serial.print("OK,SERVO,");
      Serial.print(servo_names[idx]);
      Serial.print(",");
      Serial.println(ang);
    }
  }
  else if (cmd == "STOP") {
    stopAll();
  }
  else if (cmd == "TEST") {
    Serial.println("Starting motor test...");
    for (int i = 0; i < 4; i++) {
      Serial.print("Testing ");
      Serial.print(motor_names[i]);
      Serial.println(" forward...");
      setMotor(i, 150);
      delay(1000);
      setMotor(i, 0);
      delay(500);
    }
    Serial.println("Motor test complete!");
  }
  else if (cmd == "STATUS") {
    Serial.print("STATUS,PCA9685:");
    Serial.print(pca_found ? "OK" : "NOT_FOUND");
    Serial.print(",Servos:[");
    for (int i = 0; i < 6; i++) {
      Serial.print(servo_angle[i], 0);
      if (i < 5) Serial.print(",");
    }
    Serial.println("]");
  }
  else if (cmd == "SCAN") {
    i2cScan();
  }
  else {
    Serial.print("ERROR,Unknown: ");
    Serial.println(cmd);
  }
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (cmd_buffer.length() > 0) {
        processCommand(cmd_buffer);
        cmd_buffer = "";
      }
    } else if (c != '\r') {
      cmd_buffer += c;
    }
  }
}
