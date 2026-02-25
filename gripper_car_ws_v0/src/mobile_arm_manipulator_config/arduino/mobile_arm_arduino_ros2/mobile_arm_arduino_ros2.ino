/*
 * Mobile Arm Manipulator - Arduino Uno with PCA9685 ROS2 Communication
 * 
 * This Arduino sketch provides serial communication with ROS2 for:
 * - Servo motor control (6 servos via PCA9685 driver)
 * - Analog sensor reading (potentiometers, force sensors)
 * - Digital sensor reading (limit switches, emergency stop)
 * - PWM output control
 * - Safety monitoring
 * - Smooth servo movement with interpolation
 * 
 * Communication Protocol:
 * Commands from ROS2: "SERVO,<index>,<angle>", "SET_ALL_SERVOS,<angles>", "DIGITAL,<pin>,<value>"
 * Data to ROS2: "ANALOG,<values>", "DIGITAL,<values>", "SERVO_POS,<positions>", "STATUS,<status>"
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 Servo Driver
Adafruit_PWMServoDriver pca(0x40);

// ===== PCA9685 channels =====
const uint8_t SERVO1_CH = 0;  // joint_1
const uint8_t SERVO2_CH = 1;  // joint_2
const uint8_t SERVO3_CH = 2;  // joint_3
const uint8_t SERVO4_CH = 4;  // joint_4
const uint8_t SERVO5_CH = 5;  // gripper_base_joint
const uint8_t SERVO6_CH = 6;  // left_gear_joint

// ===== Servo timing =====
const float    SERVO_FREQ_HZ = 50.0;     // 50 Hz
const uint16_t SERVO_MIN_US  = 600;      // tune per servo
const uint16_t SERVO_MAX_US  = 2400;     // tune per servo

// ===== Speed / smoothness =====
const int   STEP_DELAY_MS = 30;          // 30 ms per step (slower = smoother)
const float STEP_DEG      = 1.0f;        // 1° per step

// Pin definitions for sensors
const int ANALOG_PINS[] = {A0, A1, A2, A3, A4, A5};  // Analog input pins
const int DIGITAL_PINS[] = {2, 4, 7, 8, 12, 13};     // Digital input pins
const int EMERGENCY_STOP_PIN = 2;

// ===== Current & target angles (deg) =====
float currentAngle1 = 90, targetAngle1 = 90;    // joint_1
float currentAngle2 = 90, targetAngle2 = 90;    // joint_2
float currentAngle3 = 90, targetAngle3 = 90;    // joint_3
float currentAngle4 = 90, targetAngle4 = 90;    // joint_4
float currentAngle5 = 90, targetAngle5 = 90;    // gripper_base_joint
float currentAngle6 = 90, targetAngle6 = 90;    // left_gear_joint

// Communication variables
String inputString = "";
bool stringComplete = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 1000;  // 1 second
bool emergencyStop = false;

// Sensor data arrays
int analogValues[6] = {0};
int digitalValues[6] = {0};

// Helper function to set servo angle via PCA9685
inline void goAngle(uint8_t ch, float angle_deg) {
  angle_deg = constrain(angle_deg, 0, 180);
  float us = SERVO_MIN_US + (angle_deg / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US);
  pca.writeMicroseconds(ch, (uint16_t)us);
}

// Helper function for smooth movement
inline bool stepToward(float &cur, float tgt, float stepDeg) {
  if (cur < tgt) { cur += stepDeg; if (cur > tgt) cur = tgt; return false; }
  if (cur > tgt) { cur -= stepDeg; if (cur < tgt) cur = tgt; return false; }
  return true; // already at target
}

// Write all servos to current angles
void writeAllServos() {
  goAngle(SERVO1_CH, currentAngle1);
  goAngle(SERVO2_CH, currentAngle2);
  goAngle(SERVO3_CH, currentAngle3);
  goAngle(SERVO4_CH, currentAngle4);
  goAngle(SERVO5_CH, currentAngle5);
  goAngle(SERVO6_CH, currentAngle6);
}

// Update all servos with smooth movement
bool updateAllServos(float stepDeg) {
  bool r1 = stepToward(currentAngle1, targetAngle1, stepDeg);
  bool r2 = stepToward(currentAngle2, targetAngle2, stepDeg);
  bool r3 = stepToward(currentAngle3, targetAngle3, stepDeg);
  bool r4 = stepToward(currentAngle4, targetAngle4, stepDeg);
  bool r5 = stepToward(currentAngle5, targetAngle5, stepDeg);
  bool r6 = stepToward(currentAngle6, targetAngle6, stepDeg);
  writeAllServos();
  return r1 && r2 && r3 && r4 && r5 && r6; // true when all reached
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Mobile Arm Arduino ROS2 Interface with PCA9685 Started");
  
  // Initialize I2C and PCA9685
  Wire.begin();
  pca.begin();
  pca.setPWMFreq(SERVO_FREQ_HZ);
  delay(1000);
  
  // Initialize digital pins
  for (int i = 0; i < 6; i++) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
  }
  
  // Emergency stop pin
  pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);
  
  // Set initial servo positions with smooth movement
  Serial.println("Initializing servos to safe positions...");
  writeAllServos();
  delay(500);
  
  Serial.println("Arduino initialization complete");
  Serial.println("ROS2 Communication Protocol Ready");
  Serial.println("Commands: SERVO,<index>,<angle> | SET_ALL_SERVOS,<angles> | DIGITAL,<pin>,<value>");
}

void loop() {
  // Check for incoming serial commands
  if (stringComplete) {
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
  
  // Update servos with smooth movement (if not in emergency stop)
  if (!emergencyStop) {
    updateAllServos(STEP_DEG);
  }
  
  // Read sensors periodically
  readSensors();
  
  // Send sensor data to ROS2
  sendSensorData();
  
  // Check emergency stop
  checkEmergencyStop();
  
  // Send heartbeat
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  delay(STEP_DELAY_MS);  // Smooth movement timing
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}

void processCommand(String command) {
  command.trim();
  command.toUpperCase();
  
  // Parse command: "SERVO,<pin>,<angle>"
  if (command.startsWith("SERVO,")) {
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    
    if (firstComma != -1 && secondComma != -1) {
      int servoIndex = command.substring(firstComma + 1, secondComma).toInt();
      float angle = command.substring(secondComma + 1).toFloat();
      
      if (servoIndex >= 0 && servoIndex < 6 && angle >= 0 && angle <= 180) {
        setServo(servoIndex, angle);
        Serial.println("OK,SERVO," + String(servoIndex) + "," + String(angle));
      } else {
        Serial.println("ERROR,INVALID_SERVO_PARAMS");
      }
    }
  }
  
  // Parse command: "DIGITAL,<pin>,<value>"
  else if (command.startsWith("DIGITAL,")) {
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    
    if (firstComma != -1 && secondComma != -1) {
      int pin = command.substring(firstComma + 1, secondComma).toInt();
      int value = command.substring(secondComma + 1).toInt();
      
      if (pin >= 0 && pin < 14) {
        digitalWrite(pin, value);
        Serial.println("OK,DIGITAL," + String(pin) + "," + String(value));
      } else {
        Serial.println("ERROR,INVALID_DIGITAL_PIN");
      }
    }
  }
  
  // Parse command: "PWM,<pin>,<value>"
  else if (command.startsWith("PWM,")) {
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    
    if (firstComma != -1 && secondComma != -1) {
      int pin = command.substring(firstComma + 1, secondComma).toInt();
      int value = command.substring(secondComma + 1).toInt();
      
      if (pin >= 0 && pin < 14 && value >= 0 && value <= 255) {
        analogWrite(pin, value);
        Serial.println("OK,PWM," + String(pin) + "," + String(value));
      } else {
        Serial.println("ERROR,INVALID_PWM_PARAMS");
      }
    }
  }
  
  // Parse command: "SET_ALL_SERVOS,<angle1>,<angle2>,<angle3>,<angle4>,<angle5>,<angle6>"
  else if (command.startsWith("SET_ALL_SERVOS,")) {
    int startIndex = command.indexOf(',') + 1;
    String angles = command.substring(startIndex);
    
    float newAngles[6];
    int angleCount = 0;
    int lastIndex = 0;
    
    for (int i = 0; i <= angles.length(); i++) {
      if (i == angles.length() || angles.charAt(i) == ',') {
        if (angleCount < 6) {
          newAngles[angleCount] = angles.substring(lastIndex, i).toFloat();
          angleCount++;
        }
        lastIndex = i + 1;
      }
    }
    
    if (angleCount == 6) {
      setAllServos(newAngles);
      Serial.println("OK,SET_ALL_SERVOS");
    } else {
      Serial.println("ERROR,INVALID_SERVO_COUNT");
    }
  }
  
  // Parse command: "GET_STATUS"
  else if (command.equals("GET_STATUS")) {
    sendStatus();
  }
  
  // Parse command: "RESET_EMERGENCY"
  else if (command.equals("RESET_EMERGENCY")) {
    emergencyStop = false;
    Serial.println("OK,EMERGENCY_RESET");
  }
  
  else {
    Serial.println("ERROR,UNKNOWN_COMMAND");
  }
}

void setServo(int index, float angle) {
  angle = constrain(angle, 0, 180);
  
  switch (index) {
    case 0: targetAngle1 = angle; break;
    case 1: targetAngle2 = angle; break;
    case 2: targetAngle3 = angle; break;
    case 3: targetAngle4 = angle; break;
    case 4: targetAngle5 = angle; break;
    case 5: targetAngle6 = angle; break;
    default: return;
  }
}

void setAllServos(float angles[]) {
  if (angles != nullptr && sizeof(angles) >= 6 * sizeof(float)) {
    targetAngle1 = constrain(angles[0], 0, 180);
    targetAngle2 = constrain(angles[1], 0, 180);
    targetAngle3 = constrain(angles[2], 0, 180);
    targetAngle4 = constrain(angles[3], 0, 180);
    targetAngle5 = constrain(angles[4], 0, 180);
    targetAngle6 = constrain(angles[5], 0, 180);
  }
}

void readSensors() {
  // Read analog sensors
  for (int i = 0; i < 6; i++) {
    analogValues[i] = analogRead(ANALOG_PINS[i]);
  }
  
  // Read digital sensors
  for (int i = 0; i < 6; i++) {
    digitalValues[i] = digitalRead(DIGITAL_PINS[i]);
  }
}

void sendSensorData() {
  // Send analog data
  Serial.print("ANALOG,");
  for (int i = 0; i < 6; i++) {
    Serial.print(analogValues[i]);
    if (i < 5) Serial.print(",");
  }
  Serial.println();
  
  // Send digital data
  Serial.print("DIGITAL,");
  for (int i = 0; i < 6; i++) {
    Serial.print(digitalValues[i]);
    if (i < 5) Serial.print(",");
  }
  Serial.println();
  
  // Send current servo positions
  Serial.print("SERVO_POS,");
  Serial.print(currentAngle1); Serial.print(",");
  Serial.print(currentAngle2); Serial.print(",");
  Serial.print(currentAngle3); Serial.print(",");
  Serial.print(currentAngle4); Serial.print(",");
  Serial.print(currentAngle5); Serial.print(",");
  Serial.print(currentAngle6);
  Serial.println();
}

void sendHeartbeat() {
  Serial.println("HEARTBEAT," + String(millis()));
}

void sendStatus() {
  Serial.print("STATUS,");
  Serial.print("EMERGENCY_STOP,");
  Serial.print(emergencyStop ? "1" : "0");
  Serial.print(",SERVO_COUNT,6,ANALOG_COUNT,6,DIGITAL_COUNT,6");
  Serial.println();
}

void checkEmergencyStop() {
  if (digitalRead(EMERGENCY_STOP_PIN) == LOW) {
    emergencyStop = true;
    // Stop all servos immediately by setting targets to current positions
    targetAngle1 = currentAngle1;
    targetAngle2 = currentAngle2;
    targetAngle3 = currentAngle3;
    targetAngle4 = currentAngle4;
    targetAngle5 = currentAngle5;
    targetAngle6 = currentAngle6;
    Serial.println("EMERGENCY_STOP,ACTIVATED");
  }
}