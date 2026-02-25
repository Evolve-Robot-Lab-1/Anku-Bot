/*
 * MOBILE BASE CONTROLLER - Arduino Uno
 * =====================================
 *
 * Simplified 4-wheel differential drive for Arduino Uno
 * NO encoders (open-loop control) - simpler and works without feedback
 *
 * Hardware:
 * - Arduino Uno
 * - 2x L298N motor drivers
 * - 4x DC motors
 *
 * Pin Mapping (Uno has limited pins):
 * -----------------------------------------
 * Motor FL: IN1=4,  IN2=7,  EN=5  (PWM)
 * Motor FR: IN1=8,  IN2=12, EN=6  (PWM)
 * Motor BL: IN1=A0, IN2=A1, EN=9  (PWM)
 * Motor BR: IN1=A2, IN2=A3, EN=10 (PWM)
 *
 * I2C (for PCA9685 arm - optional):
 * - SDA = A4
 * - SCL = A5
 *
 * Serial Protocol (115200 baud):
 *   VEL,<linear_x>,<angular_z>  - Velocity command (m/s, rad/s)
 *   MOT,<idx>,<pwm>             - Direct motor control (-255 to 255)
 *   STOP                        - Emergency stop
 *   STATUS                      - Get status
 *   TEST                        - Run motor test sequence
 */

// =====================================================
// CONFIGURATION
// =====================================================

#define SERIAL_BAUD 115200

// Robot parameters
#define WHEEL_RADIUS 0.075  // meters
#define WHEEL_BASE 0.35     // meters

// Timing
#define CMD_TIMEOUT 2000        // milliseconds
#define HEARTBEAT_INTERVAL 1000 // milliseconds

// =====================================================
// PIN DEFINITIONS - Arduino Uno
// =====================================================

// Motor FL (Front Left) - L298N #1 OUT1/OUT2
#define M_FL_IN1 4
#define M_FL_IN2 7
#define M_FL_EN  5   // PWM

// Motor FR (Front Right) - L298N #1 OUT3/OUT4
#define M_FR_IN1 8
#define M_FR_IN2 12
#define M_FR_EN  6   // PWM

// Motor BL (Back Left) - L298N #2 OUT1/OUT2
#define M_BL_IN1 A0  // 14
#define M_BL_IN2 A1  // 15
#define M_BL_EN  9   // PWM

// Motor BR (Back Right) - L298N #2 OUT3/OUT4
#define M_BR_IN1 A2  // 16
#define M_BR_IN2 A3  // 17
#define M_BR_EN  10  // PWM

// Motor arrays for easy iteration
const int motor_in1[4] = { M_FL_IN1, M_FR_IN1, M_BL_IN1, M_BR_IN1 };
const int motor_in2[4] = { M_FL_IN2, M_FR_IN2, M_BL_IN2, M_BR_IN2 };
const int motor_en[4]  = { M_FL_EN,  M_FR_EN,  M_BL_EN,  M_BR_EN };

// Motor direction correction (adjust if motors spin wrong way)
// 1 = normal, -1 = reversed
const int motor_dir[4] = { 1, 1, 1, 1 };

// Motor calibration (adjust for motor speed differences)
float motor_cal[4] = { 1.0, 1.0, 1.0, 1.0 };

// =====================================================
// GLOBAL VARIABLES
// =====================================================

unsigned long last_cmd_time = 0;
unsigned long last_heartbeat_time = 0;
String cmd_buffer = "";
int current_pwm[4] = { 0, 0, 0, 0 };

// =====================================================
// MOTOR CONTROL FUNCTIONS
// =====================================================

void setupMotors() {
  for (int i = 0; i < 4; i++) {
    pinMode(motor_in1[i], OUTPUT);
    pinMode(motor_in2[i], OUTPUT);
    pinMode(motor_en[i], OUTPUT);

    digitalWrite(motor_in1[i], LOW);
    digitalWrite(motor_in2[i], LOW);
    analogWrite(motor_en[i], 0);
  }
  Serial.println("STATUS,Motors initialized");
}

void setMotor(int idx, int pwm) {
  if (idx < 0 || idx > 3) return;

  // Apply calibration and direction
  float scaled = pwm * motor_cal[idx] * motor_dir[idx];
  int final_pwm = constrain((int)scaled, -255, 255);

  int magnitude = abs(final_pwm);

  if (final_pwm > 0) {
    // Forward
    digitalWrite(motor_in1[idx], HIGH);
    digitalWrite(motor_in2[idx], LOW);
  } else if (final_pwm < 0) {
    // Reverse
    digitalWrite(motor_in1[idx], LOW);
    digitalWrite(motor_in2[idx], HIGH);
  } else {
    // Stop (brake)
    digitalWrite(motor_in1[idx], LOW);
    digitalWrite(motor_in2[idx], LOW);
  }

  analogWrite(motor_en[idx], magnitude);
  current_pwm[idx] = pwm;
}

void stopAllMotors() {
  for (int i = 0; i < 4; i++) {
    setMotor(i, 0);
  }
  Serial.println("STATUS,All motors stopped");
}

// =====================================================
// VELOCITY CONTROL (Open Loop)
// =====================================================

void setVelocity(float linear_x, float angular_z) {
  // Differential drive kinematics
  float v_left = linear_x - (angular_z * WHEEL_BASE / 2.0);
  float v_right = linear_x + (angular_z * WHEEL_BASE / 2.0);

  // Convert to PWM (rough mapping: 0.5 m/s = 255 PWM)
  // Adjust PWM_SCALE based on your motors
  const float PWM_SCALE = 255.0 / 0.5;  // max speed ~0.5 m/s

  int pwm_left = constrain((int)(v_left * PWM_SCALE), -255, 255);
  int pwm_right = constrain((int)(v_right * PWM_SCALE), -255, 255);

  // Set all 4 motors (left side same, right side same)
  setMotor(0, pwm_left);   // FL
  setMotor(1, pwm_right);  // FR
  setMotor(2, pwm_left);   // BL
  setMotor(3, pwm_right);  // BR

  last_cmd_time = millis();
}

// =====================================================
// TEST SEQUENCE
// =====================================================

void runMotorTest() {
  Serial.println("STATUS,Starting motor test...");

  const char* names[4] = {"FL", "FR", "BL", "BR"};

  for (int i = 0; i < 4; i++) {
    Serial.print("STATUS,Testing motor ");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(names[i]);
    Serial.println(") forward...");

    setMotor(i, 150);
    delay(1000);
    setMotor(i, 0);
    delay(500);

    Serial.print("STATUS,Testing motor ");
    Serial.print(i);
    Serial.println(" reverse...");

    setMotor(i, -150);
    delay(1000);
    setMotor(i, 0);
    delay(500);
  }

  Serial.println("STATUS,Motor test complete");
}

// =====================================================
// SERIAL COMMUNICATION
// =====================================================

void processCommand(String cmd) {
  cmd.trim();

  if (cmd.startsWith("VEL,")) {
    // VEL,<linear>,<angular>
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);

    if (idx2 > idx1) {
      float linear_x = cmd.substring(idx1 + 1, idx2).toFloat();
      float angular_z = cmd.substring(idx2 + 1).toFloat();
      setVelocity(linear_x, angular_z);
      Serial.println("OK,VEL");
    }
  }
  else if (cmd.startsWith("MOT,")) {
    // MOT,<idx>,<pwm>
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);

    if (idx2 > idx1) {
      int motor_idx = cmd.substring(idx1 + 1, idx2).toInt();
      int pwm = cmd.substring(idx2 + 1).toInt();

      if (motor_idx >= 0 && motor_idx < 4) {
        setMotor(motor_idx, pwm);
        last_cmd_time = millis();
        Serial.print("OK,MOT,");
        Serial.print(motor_idx);
        Serial.print(",");
        Serial.println(pwm);
      }
    }
  }
  else if (cmd == "STOP") {
    stopAllMotors();
  }
  else if (cmd == "TEST") {
    runMotorTest();
  }
  else if (cmd == "STATUS") {
    Serial.print("STATUS,PWM:[");
    for (int i = 0; i < 4; i++) {
      Serial.print(current_pwm[i]);
      if (i < 3) Serial.print(",");
    }
    Serial.println("]");
  }
  else if (cmd.startsWith("CAL,")) {
    // CAL,<fl>,<fr>,<bl>,<br>
    int i1 = cmd.indexOf(',');
    int i2 = cmd.indexOf(',', i1 + 1);
    int i3 = cmd.indexOf(',', i2 + 1);
    int i4 = cmd.indexOf(',', i3 + 1);

    if (i4 > i3) {
      motor_cal[0] = cmd.substring(i1 + 1, i2).toFloat();
      motor_cal[1] = cmd.substring(i2 + 1, i3).toFloat();
      motor_cal[2] = cmd.substring(i3 + 1, i4).toFloat();
      motor_cal[3] = cmd.substring(i4 + 1).toFloat();
      Serial.println("OK,CAL");
    }
  }
  else if (cmd.startsWith("DIR,")) {
    // DIR,<idx>,<1 or -1> - flip motor direction
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);
    if (idx2 > idx1) {
      int m = cmd.substring(idx1 + 1, idx2).toInt();
      int d = cmd.substring(idx2 + 1).toInt();
      if (m >= 0 && m < 4 && (d == 1 || d == -1)) {
        // Can't modify const, but user can adjust in code
        Serial.print("STATUS,Motor ");
        Serial.print(m);
        Serial.print(" direction noted as ");
        Serial.println(d);
      }
    }
  }
  else {
    Serial.print("ERROR,Unknown: ");
    Serial.println(cmd);
  }
}

void readSerial() {
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

// =====================================================
// TIMEOUT & HEARTBEAT
// =====================================================

void checkTimeout() {
  if (millis() - last_cmd_time > CMD_TIMEOUT) {
    bool any_moving = false;
    for (int i = 0; i < 4; i++) {
      if (current_pwm[i] != 0) {
        any_moving = true;
        break;
      }
    }
    if (any_moving) {
      stopAllMotors();
      Serial.println("STATUS,Timeout - motors stopped");
    }
  }
}

void sendHeartbeat() {
  if (millis() - last_heartbeat_time >= HEARTBEAT_INTERVAL) {
    Serial.print("HEARTBEAT,");
    Serial.println(millis());
    last_heartbeat_time = millis();
  }
}

// =====================================================
// MAIN
// =====================================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);

  setupMotors();

  last_cmd_time = millis();
  last_heartbeat_time = millis();

  Serial.println("=================================");
  Serial.println("Mobile Base Controller - Uno");
  Serial.println("=================================");
  Serial.println("Commands:");
  Serial.println("  VEL,<linear>,<angular> - Set velocity");
  Serial.println("  MOT,<0-3>,<-255 to 255> - Direct motor");
  Serial.println("  STOP - Emergency stop");
  Serial.println("  TEST - Run motor test");
  Serial.println("  STATUS - Get current state");
  Serial.println("=================================");
}

void loop() {
  readSerial();
  checkTimeout();
  sendHeartbeat();
}
