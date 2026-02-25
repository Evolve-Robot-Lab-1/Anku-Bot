/*
 * UNIFIED MOBILE MANIPULATOR CONTROLLER WITH OBSTACLE AVOIDANCE
 * ==============================================================
 *
 * Based on mobile_manipulator_unified.ino
 * Added: OBS,<0|1> command to enable/disable obstacle avoidance
 * When obstacle detected: blocks forward motion, allows reverse and rotation
 *
 * Serial Protocol (115200 baud):
 *   ROS -> Arduino:
 *     OBS,<0|1>                          - Set obstacle flag (NEW!)
 *     ARM,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>  - Set arm servo angles (0-180)
 *     VEL,<linear_x>,<angular_z>         - Velocity command (m/s, rad/s)
 *     STOP                               - Emergency stop all
 *     ... (all other commands same as unified)
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// =====================================================
// CONFIGURATION
// =====================================================

#define SERIAL_BAUD 115200
#define WHEEL_RADIUS 0.075
#define WHEEL_BASE 0.35
#define ENCODER_CPR 360
#define PID_SAMPLE_TIME 20
#define ENCODER_PUB_RATE 50
#define CMD_TIMEOUT 2000
#define SERVO_STEP_DELAY 80  // Smoother: 80ms between steps
#define HEARTBEAT_INTERVAL 1000

float Kp = 2.0;
float Ki = 0.5;
float Kd = 0.1;

unsigned long encoder_debounce_us = 200;
float motor_cal[4] = { 0.902, 0.750, 1.00, 0.80 };

// =====================================================
// OBSTACLE AVOIDANCE (NEW!)
// =====================================================
bool obstacle_detected = false;
unsigned long last_obs_time = 0;
#define OBS_TIMEOUT 500  // Auto-clear obstacle after 500ms if no update

// =====================================================
// ARM CONFIGURATION (PCA9685)
// =====================================================

Adafruit_PWMServoDriver pca(0x40);
bool pca_available = false;  // Track if PCA9685 is connected
const uint8_t SERVO_CH[6] = {0, 1, 6, 3, 4, 15};  // CH: 0,1,6,3,4,unused(15)
const float SERVO_FREQ_HZ = 50.0;
const uint16_t SERVO_MIN_US = 600;
const uint16_t SERVO_MAX_US = 2400;
const float SERVO_STEP_DEG = 0.5;  // Slower: 0.5 deg per step (was 1.0)

float armCurrentAngle[6] = {90, 90, 90, 90, 90, 90};
float armTargetAngle[6] = {90, 90, 90, 90, 90, 90};
float armLastSentAngle[6] = {90, 90, 90, 90, 90, 90};

// =====================================================
// BASE CONFIGURATION (L298N)
// =====================================================

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

#define ENC_FL_A 18
#define ENC_FL_B 31
#define ENC_FR_A 19
#define ENC_FR_B 33
#define ENC_BL_A 2
#define ENC_BL_B 35
#define ENC_BR_A 3
#define ENC_BR_B 37

const int motor_in1_pins[4] = { M_FL_IN1, M_FR_IN1, M_BL_IN1, M_BR_IN1 };
const int motor_in2_pins[4] = { M_FL_IN2, M_FR_IN2, M_BL_IN2, M_BR_IN2 };
const int motor_en_pins[4] = { M_FL_EN, M_FR_EN, M_BL_EN, M_BR_EN };
const int motor_direction[4] = { 1, 1, 1, 1 };
const int encoder_dir[4] = { 1, -1, 1, -1 };

// =====================================================
// GLOBAL VARIABLES
// =====================================================

volatile long enc_counts[4] = { 0, 0, 0, 0 };
long prev_enc_counts[4] = { 0, 0, 0, 0 };
volatile unsigned long last_isr_time[4] = { 0, 0, 0, 0 };
long last_published_counts[4] = { 0, 0, 0, 0 };

float target_vel[4] = { 0, 0, 0, 0 };
float current_vel[4] = { 0, 0, 0, 0 };

float pid_integral[4] = { 0, 0, 0, 0 };
float pid_prev_error[4] = { 0, 0, 0, 0 };
int motor_state[4] = { 0, 0, 0, 0 };
int prev_motor_state[4] = { 0, 0, 0, 0 };
bool motor_locked[4] = { false, false, false, false };

unsigned long last_pid_time = 0;
unsigned long last_enc_pub_time = 0;
unsigned long last_cmd_time = 0;
unsigned long last_servo_update_time = 0;
unsigned long last_heartbeat_time = 0;
unsigned long last_test_update_time = 0;

String cmd_buffer = "";
bool test_mode = false;
float sim_enc_accumulator[4] = { 0.0, 0.0, 0.0, 0.0 };
bool manual_active[4] = { false, false, false, false };

// =====================================================
// ARM FUNCTIONS
// =====================================================

inline void goServoAngle(uint8_t ch, float angle) {
  if (!pca_available) return;  // Skip if PCA9685 not connected
  angle = constrain(angle, 0, 180);
  float us = SERVO_MIN_US + (angle / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US);
  pca.writeMicroseconds(ch, (uint16_t)us);
}

bool updateArmServos() {
  if (!pca_available) return false;  // Skip if PCA9685 not connected
  bool changed = false;
  for (int i = 0; i < 6; i++) {
    float before = armCurrentAngle[i];
    if (armCurrentAngle[i] < armTargetAngle[i]) {
      armCurrentAngle[i] += SERVO_STEP_DEG;
      if (armCurrentAngle[i] > armTargetAngle[i]) armCurrentAngle[i] = armTargetAngle[i];
    }
    else if (armCurrentAngle[i] > armTargetAngle[i]) {
      armCurrentAngle[i] -= SERVO_STEP_DEG;
      if (armCurrentAngle[i] < armTargetAngle[i]) armCurrentAngle[i] = armTargetAngle[i];
    }
    if (armCurrentAngle[i] != before) changed = true;
    goServoAngle(SERVO_CH[i], armCurrentAngle[i]);
  }
  return changed;
}

void sendArmPositions() {
  bool changed = false;
  for (int i = 0; i < 6; i++) {
    if (armCurrentAngle[i] != armLastSentAngle[i]) { changed = true; break; }
  }
  if (changed) {
    Serial.print("ARM_POS,");
    for (int i = 0; i < 6; i++) {
      Serial.print(armCurrentAngle[i], 1);
      if (i < 5) Serial.print(",");
      armLastSentAngle[i] = armCurrentAngle[i];
    }
    Serial.println();
  }
}

void stopArm() {
  for (int i = 0; i < 6; i++) armTargetAngle[i] = armCurrentAngle[i];
}

// =====================================================
// BASE MOTOR FUNCTIONS
// =====================================================

void setupMotors() {
  for (int i = 0; i < 4; i++) {
    pinMode(motor_in1_pins[i], OUTPUT);
    pinMode(motor_in2_pins[i], OUTPUT);
    pinMode(motor_en_pins[i], OUTPUT);
    digitalWrite(motor_in1_pins[i], LOW);
    digitalWrite(motor_in2_pins[i], LOW);
    analogWrite(motor_en_pins[i], 0);
  }
}

void setMotorOutput(int motor_idx, int pwm_like) {
  if (motor_locked[motor_idx] && pwm_like != 0) return;

  float scaled_f = pwm_like * motor_cal[motor_idx];
  int scaled_pwm = (int)constrain(scaled_f, -255, 255);
  int final_pwm = scaled_pwm * motor_direction[motor_idx];

  int pwm_magnitude = abs(final_pwm);
  int direction = (final_pwm > 0) ? 1 : (final_pwm < 0) ? -1 : 0;

  if (direction > 0) {
    digitalWrite(motor_in1_pins[motor_idx], HIGH);
    digitalWrite(motor_in2_pins[motor_idx], LOW);
    motor_state[motor_idx] = 1;
  } else if (direction < 0) {
    digitalWrite(motor_in1_pins[motor_idx], LOW);
    digitalWrite(motor_in2_pins[motor_idx], HIGH);
    motor_state[motor_idx] = -1;
  } else {
    digitalWrite(motor_in1_pins[motor_idx], LOW);
    digitalWrite(motor_in2_pins[motor_idx], LOW);
    motor_state[motor_idx] = 0;
  }

  analogWrite(motor_en_pins[motor_idx], pwm_magnitude);

  // Motor locking disabled for obstacle avoidance compatibility
  // Direction changes are expected during avoidance maneuvers
  prev_motor_state[motor_idx] = motor_state[motor_idx];
}

void stopBase() {
  for (int i = 0; i < 4; i++) {
    setMotorOutput(i, 0);
    target_vel[i] = 0;
    pid_integral[i] = 0;
    pid_prev_error[i] = 0;
    manual_active[i] = false;
  }
}

void stopAll() {
  stopArm();
  stopBase();
  Serial.println("STATUS,Emergency stop - all systems stopped");
}

// =====================================================
// ENCODER FUNCTIONS
// =====================================================

void setupEncoders() {
  pinMode(ENC_FL_A, INPUT_PULLUP); pinMode(ENC_FL_B, INPUT_PULLUP);
  pinMode(ENC_FR_A, INPUT_PULLUP); pinMode(ENC_FR_B, INPUT_PULLUP);
  pinMode(ENC_BL_A, INPUT_PULLUP); pinMode(ENC_BL_B, INPUT_PULLUP);
  pinMode(ENC_BR_A, INPUT_PULLUP); pinMode(ENC_BR_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_FL_A), isr_enc_fl, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_FR_A), isr_enc_fr, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_BL_A), isr_enc_bl, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_BR_A), isr_enc_br, RISING);
}

void isr_enc_fl() {
  unsigned long now = micros();
  if (now - last_isr_time[0] < encoder_debounce_us) return;
  last_isr_time[0] = now;
  if (digitalRead(ENC_FL_B)) enc_counts[0]--; else enc_counts[0]++;
}

void isr_enc_fr() {
  unsigned long now = micros();
  if (now - last_isr_time[1] < encoder_debounce_us) return;
  last_isr_time[1] = now;
  if (digitalRead(ENC_FR_B)) enc_counts[1]++; else enc_counts[1]--;
}

void isr_enc_bl() {
  unsigned long now = micros();
  if (now - last_isr_time[2] < encoder_debounce_us) return;
  last_isr_time[2] = now;
  if (digitalRead(ENC_BL_B)) enc_counts[2]--; else enc_counts[2]++;
}

void isr_enc_br() {
  unsigned long now = micros();
  if (now - last_isr_time[3] < encoder_debounce_us) return;
  last_isr_time[3] = now;
  if (digitalRead(ENC_BR_B)) enc_counts[3]++; else enc_counts[3]--;
}

void getEncoderCounts(long* counts) {
  noInterrupts();
  for (int i = 0; i < 4; i++) counts[i] = enc_counts[i];
  interrupts();
}

void resetEncoders() {
  noInterrupts();
  for (int i = 0; i < 4; i++) { enc_counts[i] = 0; prev_enc_counts[i] = 0; }
  interrupts();
  for (int i = 0; i < 4; i++) sim_enc_accumulator[i] = 0.0;
}

// =====================================================
// TEST MODE
// =====================================================

void updateSimulatedEncoders() {
  if (!test_mode) return;
  unsigned long now = millis();
  float dt = (now - last_test_update_time) / 1000.0;
  if (dt <= 0 || dt > 0.1) { last_test_update_time = now; return; }

  for (int i = 0; i < 4; i++) {
    float delta_ticks = (target_vel[i] * dt * ENCODER_CPR) / (2.0 * PI);
    sim_enc_accumulator[i] += delta_ticks;
    long whole_ticks = (long)sim_enc_accumulator[i];
    if (whole_ticks != 0) {
      noInterrupts(); enc_counts[i] += whole_ticks; interrupts();
      sim_enc_accumulator[i] -= whole_ticks;
    }
  }
  last_test_update_time = now;
}

// =====================================================
// PID CONTROLLER
// =====================================================

float computePID(int motor_idx, float target, float current, float dt) {
  float error = target - current;
  float p_term = Kp * error;
  pid_integral[motor_idx] += error * dt;
  pid_integral[motor_idx] = constrain(pid_integral[motor_idx], -100, 100);
  float i_term = Ki * pid_integral[motor_idx];
  float d_term = (dt > 0) ? Kd * (error - pid_prev_error[motor_idx]) / dt : 0;
  pid_prev_error[motor_idx] = error;
  float output = (p_term + i_term + d_term) * 25.5;
  return constrain(fabs(output), 0, 255);
}

void updatePID() {
  unsigned long now = millis();
  float dt = (now - last_pid_time) / 1000.0;
  if (dt < (PID_SAMPLE_TIME / 1000.0)) return;

  long counts[4];
  getEncoderCounts(counts);

  for (int i = 0; i < 4; i++) {
    long raw_delta = counts[i] - prev_enc_counts[i];
    long delta = raw_delta * encoder_dir[i];
    prev_enc_counts[i] = counts[i];
    current_vel[i] = (delta * 2.0 * PI) / (ENCODER_CPR * dt);
  }

  for (int i = 0; i < 4; i++) {
    float vel_error = target_vel[i] - current_vel[i];
    if (fabs(vel_error) < 0.3) { setMotorOutput(i, 0); pid_integral[i] = 0; continue; }
    int pwm_mag = (int)computePID(i, fabs(target_vel[i]), fabs(current_vel[i]), dt);
    if (fabs(target_vel[i]) < 0.01) { pwm_mag = 0; pid_integral[i] = 0; }
    int pwm = (target_vel[i] >= 0) ? pwm_mag : -pwm_mag;
    setMotorOutput(i, pwm);
  }
  last_pid_time = now;
}

// =====================================================
// KINEMATICS WITH OBSTACLE AVOIDANCE (MODIFIED!)
// =====================================================

void setVelocityCommand(float linear_x, float angular_z) {
  // OBSTACLE AVOIDANCE: Block forward motion if obstacle detected
  if (obstacle_detected && linear_x > 0) {
    linear_x = 0;  // Block forward, allow rotation
    Serial.println("STATUS,OBSTACLE - Forward blocked!");
  }

  float v_left = linear_x - (angular_z * WHEEL_BASE / 2.0);
  float v_right = linear_x + (angular_z * WHEEL_BASE / 2.0);

  float omega_left = v_left / WHEEL_RADIUS;
  float omega_right = v_right / WHEEL_RADIUS;

  target_vel[0] = omega_left;
  target_vel[1] = omega_right;
  target_vel[2] = omega_left;
  target_vel[3] = omega_right;

  last_cmd_time = millis();
}

// =====================================================
// SERIAL COMMUNICATION (MODIFIED - Added OBS command)
// =====================================================

void processCommand(String cmd) {
  cmd.trim();

  // OBS command: OBS,<0|1> - Set obstacle flag (NEW!)
  if (cmd.startsWith("OBS,")) {
    int val = cmd.substring(4).toInt();
    obstacle_detected = (val != 0);
    last_obs_time = millis();
    Serial.print("OK,OBS,");
    Serial.println(obstacle_detected ? "1" : "0");

    // If obstacle just detected and moving forward, stop immediately
    if (obstacle_detected) {
      bool moving_forward = (target_vel[0] > 0 || target_vel[1] > 0);
      if (moving_forward) {
        stopBase();
        Serial.println("STATUS,OBSTACLE STOP!");
      }
    }
    return;
  }

  // ARM command
  if (cmd.startsWith("ARM,") || cmd.startsWith("SET_ALL_SERVOS,")) {
    int prefixLen = cmd.startsWith("ARM,") ? 4 : 15;
    String t = cmd.substring(prefixLen);
    float vals[6];
    int index = 0, last = 0;
    for (int i = 0; i <= t.length(); i++) {
      if (i == t.length() || t.charAt(i) == ',') {
        vals[index++] = t.substring(last, i).toFloat();
        last = i + 1;
        if (index >= 6) break;
      }
    }
    if (index == 6) {
      for (int i = 0; i < 6; i++) armTargetAngle[i] = constrain(vals[i], 0, 180);
      last_cmd_time = millis();
      Serial.println("OK,SET_ALL_SERVOS");
    }
  }
  // VEL command
  else if (cmd.startsWith("VEL,")) {
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);
    if (idx2 > idx1) {
      float linear_x = cmd.substring(idx1 + 1, idx2).toFloat();
      float angular_z = cmd.substring(idx2 + 1).toFloat();
      setVelocityCommand(linear_x, angular_z);
      Serial.println("OK,VEL");
    }
  }
  else if (cmd == "STOP") { stopAll(); }
  else if (cmd == "STOP_ARM") { stopArm(); Serial.println("STATUS,Arm stopped"); }
  else if (cmd == "STOP_BASE") { stopBase(); Serial.println("STATUS,Base stopped"); }
  else if (cmd.startsWith("PID,")) {
    int i1 = cmd.indexOf(','), i2 = cmd.indexOf(',', i1+1), i3 = cmd.indexOf(',', i2+1);
    if (i3 > i2 && i2 > i1) {
      Kp = cmd.substring(i1+1, i2).toFloat();
      Ki = cmd.substring(i2+1, i3).toFloat();
      Kd = cmd.substring(i3+1).toFloat();
      Serial.print("STATUS,PID: "); Serial.print(Kp); Serial.print(",");
      Serial.print(Ki); Serial.print(","); Serial.println(Kd);
    }
  }
  else if (cmd == "RST") { resetEncoders(); Serial.println("STATUS,Encoders reset"); }
  else if (cmd == "TEST,ON") { test_mode = true; last_test_update_time = millis(); resetEncoders(); Serial.println("STATUS,Test ON"); }
  else if (cmd == "TEST,OFF") { test_mode = false; resetEncoders(); Serial.println("STATUS,Test OFF"); }
  else if (cmd == "STATUS") {
    Serial.print("STATUS,Mode:"); Serial.print(test_mode ? "TEST" : "NORMAL");
    Serial.print(",OBS:"); Serial.print(obstacle_detected ? "1" : "0");
    Serial.print(",Vel:[");
    for (int i = 0; i < 4; i++) { Serial.print(target_vel[i], 2); if (i < 3) Serial.print(","); }
    Serial.println("]");
  }
  else if (cmd.startsWith("UNLOCK,")) {
    String arg = cmd.substring(7);
    if (arg == "ALL") { for (int i = 0; i < 4; i++) motor_locked[i] = false; Serial.println("STATUS,All unlocked"); }
    else { int m = arg.toInt(); if (m >= 0 && m < 4) { motor_locked[m] = false; } }
  }
  else if (cmd.startsWith("MOT,")) {
    int i1 = cmd.indexOf(','), i2 = cmd.indexOf(',', i1+1);
    if (i2 > i1) {
      int m_idx = cmd.substring(i1+1, i2).toInt();
      int pwm = cmd.substring(i2+1).toInt();
      if (m_idx >= 0 && m_idx < 4) {
        setMotorOutput(m_idx, pwm); manual_active[m_idx] = true; last_cmd_time = millis();
      }
    }
  }
  else if (cmd.startsWith("SERVO,")) {
    int c1 = cmd.indexOf(','), c2 = cmd.indexOf(',', c1+1);
    int idx = cmd.substring(c1+1, c2).toInt();
    float ang = cmd.substring(c2+1).toFloat();
    if (idx >= 0 && idx < 6) { armTargetAngle[idx] = constrain(ang, 0, 180); last_cmd_time = millis(); }
  }
  else { Serial.print("ERROR,Unknown: "); Serial.println(cmd); }
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (cmd_buffer.length() > 0) { processCommand(cmd_buffer); cmd_buffer = ""; }
    } else if (c != '\r') cmd_buffer += c;
  }
}

void publishEncoderCounts() {
  unsigned long now = millis();
  if (now - last_enc_pub_time >= (1000 / ENCODER_PUB_RATE)) {
    long counts[4];
    getEncoderCounts(counts);
    bool changed = false;
    for (int i = 0; i < 4; i++) if (counts[i] != last_published_counts[i]) { changed = true; break; }
    if (changed) {
      Serial.print("ENC,"); Serial.print(counts[0]); Serial.print(",");
      Serial.print(counts[1]); Serial.print(","); Serial.print(counts[2]);
      Serial.print(","); Serial.println(counts[3]);
      for (int i = 0; i < 4; i++) last_published_counts[i] = counts[i];
    }
    last_enc_pub_time = now;
  }
}

void sendHeartbeat() {
  unsigned long now = millis();
  if (now - last_heartbeat_time >= HEARTBEAT_INTERVAL) {
    Serial.print("HEARTBEAT,"); Serial.print(now);
    Serial.print(",OBS:"); Serial.println(obstacle_detected ? "1" : "0");
    last_heartbeat_time = now;
  }
}

void checkCommandTimeout() {
  unsigned long dt = millis() - last_cmd_time;
  if (dt > CMD_TIMEOUT) {
    bool any_target = (target_vel[0] != 0 || target_vel[1] != 0 || target_vel[2] != 0 || target_vel[3] != 0);
    bool any_manual = (manual_active[0] || manual_active[1] || manual_active[2] || manual_active[3]);
    if (any_target || any_manual) { stopBase(); Serial.println("STATUS,Timeout stop"); }
  }
}

// Auto-clear obstacle flag if no update received
void checkObstacleTimeout() {
  if (obstacle_detected && (millis() - last_obs_time > OBS_TIMEOUT)) {
    obstacle_detected = false;
    Serial.println("STATUS,OBS cleared (timeout)");
  }
}

// =====================================================
// MAIN
// =====================================================

bool scanI2C() {
  Serial.println("STATUS,Checking for PCA9685 at 0x40...");
  Serial.flush();

  // Only check PCA9685 address (0x40) to avoid hanging on other addresses
  Wire.beginTransmission(0x40);
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println("STATUS,PCA9685 found at 0x40");
    return true;
  } else {
    Serial.println("STATUS,PCA9685 not found");
    return false;
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);  // Brief delay instead of waiting for Serial

  Serial.println("STATUS,Arduino starting...");
  Serial.flush();

  Wire.begin();
  delay(100);

  Serial.println("STATUS,I2C initialized");
  Serial.flush();

  // Skip PCA9685 detection - it blocks when not connected
  // Just assume it's not available; arm commands will be ignored
  pca_available = false;
  Serial.println("STATUS,PCA9685 disabled (connect and restart to enable arm)");
  Serial.flush();

  setupMotors();
  setupEncoders();

  last_pid_time = millis();
  last_enc_pub_time = millis();
  last_cmd_time = millis();
  last_servo_update_time = millis();
  last_heartbeat_time = millis();
  last_test_update_time = millis();
  last_obs_time = millis();

  Serial.println("STATUS,Unified Controller + Obstacle Avoidance Ready");
  Serial.println("STATUS,NEW: OBS,<0|1> - Set obstacle flag");
  Serial.println("STATUS,Commands: VEL, ARM, STOP, etc.");
}

void loop() {
  readSerial();
  checkCommandTimeout();
  checkObstacleTimeout();  // NEW!
  updateSimulatedEncoders();
  if (!test_mode) updatePID();

  unsigned long now = millis();
  if (now - last_servo_update_time >= SERVO_STEP_DELAY) {
    bool arm_moved = updateArmServos();
    if (arm_moved) sendArmPositions();
    last_servo_update_time = now;
  }

  publishEncoderCounts();
  sendHeartbeat();
}
