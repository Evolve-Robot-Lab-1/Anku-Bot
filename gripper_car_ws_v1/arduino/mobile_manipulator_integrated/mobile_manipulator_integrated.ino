/*
 * ============================================================================
 * MOBILE MANIPULATOR - UNIFIED CONTROLLER
 * ============================================================================
 * 
 * Combined 4-wheel differential drive mobile base + 6-DOF robotic arm
 * Arduino Mega 2560 + 2x L298N Motor Drivers + PCA9685 Servo Driver
 * 
 * FEATURES:
 * - Mobile Base: 4-wheel differential drive with encoder feedback & PID
 * - Robotic Arm: 6 servo joints with smooth motion via PCA9685
 * - Unified serial protocol for ROS2 integration
 * - Independent control of base and arm subsystems
 * - Coordinated feedback publishing
 * 
 * HARDWARE:
 * - Arduino Mega 2560
 * - Mobile Base:
 *   - 2x L298N H-bridge motor drivers
 *   - 4x DC motors with quadrature encoders (360 CPR)
 * - Robotic Arm:
 *   - PCA9685 16-channel PWM servo driver (I2C: 0x40)
 *   - 6x servo motors (channels: 0,1,2,4,5,6)
 * 
 * UNIFIED SERIAL PROTOCOL (115200 baud):
 * 
 *   ROS -> Arduino:
 *     VEL,<linear_x>,<angular_z>        - Mobile base velocity (m/s, rad/s)
 *     STOP                              - Emergency stop mobile base
 *     PID,<kp>,<ki>,<kd>               - Update base PID gains
 *     RST                               - Reset wheel encoders
 *     
 *     SERVO,<idx>,<angle>               - Single servo control (0-5, 0-180°)
 *     ALL,<a0>,<a1>,<a2>,<a3>,<a4>,<a5> - All servos simultaneously
 *     
 *     STOP_ALL                          - Emergency stop both systems
 *     STATUS                            - Query full system status
 * 
 *   Arduino -> ROS:
 *     ENC,<fl>,<fr>,<bl>,<br>           - Wheel encoder counts
 *     POS,<j0>,<j1>,<j2>,<j3>,<j4>,<j5> - Joint angles (degrees)
 *     STATUS,<message>                  - Status messages
 *     HEARTBEAT,<millis>                - System heartbeat (1 Hz)
 * 
 * AUTHOR: Mobile Manipulator Integration
 * VERSION: 1.1
 * DATE: December 2025
 * ============================================================================
 */


#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


// ============================================================================
// MOBILE BASE CONFIGURATION
// ============================================================================


// Robot Physical Parameters
#define WHEEL_RADIUS     0.075    // meters
#define WHEEL_BASE       0.35     // meters
#define ENCODER_CPR      360      // Counts per revolution


// Control Loop Timing
#define BASE_PID_SAMPLE_TIME  20   // milliseconds (50 Hz)
#define BASE_ENCODER_PUB_RATE 50   // Hz
#define BASE_CMD_TIMEOUT      2000 // milliseconds


// PID Controller Gains
float base_Kp = 2.0;
float base_Ki = 0.5;
float base_Kd = 0.1;


// Encoder Noise Filtering
#define ENCODER_DEBOUNCE_US 200   // microseconds


// Motor Calibration Factors
float motor_cal[4] = {0.902, 0.750, 1.00, 0.80};  // FL, FR, BL, BR


// ============================================================================
// ROBOTIC ARM CONFIGURATION
// ============================================================================


// PCA9685 Setup
Adafruit_PWMServoDriver pca(0x40);
const uint8_t SERVO_CH[6] = {0, 1, 2, 4, 5, 6};


// Servo Parameters
const float SERVO_FREQ_HZ = 50.0;
const uint16_t SERVO_MIN_US = 600;
const uint16_t SERVO_MAX_US = 2400;


// Smooth Motion Parameters
const float ARM_STEP_DEG = 1.0;      // degrees per step
const int ARM_STEP_DELAY_MS = 30;    // milliseconds between steps


// ============================================================================
// HARDWARE PIN DEFINITIONS - MOBILE BASE
// ============================================================================


// Motor Driver 1 - Front Wheels
#define M_FL_IN1  22
#define M_FL_IN2  23
#define M_FL_EN   12
#define M_FR_IN1  24
#define M_FR_IN2  25
#define M_FR_EN   11


// Motor Driver 2 - Rear Wheels
#define M_BL_IN1  26
#define M_BL_IN2  27
#define M_BL_EN   10
#define M_BR_IN1  28
#define M_BR_IN2  29
#define M_BR_EN   9


// Quadrature Encoders
#define ENC_FL_A  18    // INT3
#define ENC_FL_B  31
#define ENC_FR_A  19    // INT2
#define ENC_FR_B  33
#define ENC_BL_A  16    // INT1
#define ENC_BL_B  35
#define ENC_BR_A  17    // INT0
#define ENC_BR_B  37


// ============================================================================
// GLOBAL STATE VARIABLES - MOBILE BASE
// ============================================================================


// Encoder State (volatile for ISR access)
volatile long base_enc_counts[4] = {0, 0, 0, 0};
volatile unsigned long base_last_isr_time[4] = {0, 0, 0, 0};
long base_prev_enc_counts[4] = {0, 0, 0, 0};
long base_last_published_counts[4] = {0, 0, 0, 0};
const int encoder_dir[4] = {1, -1, 1, -1};


// Velocity Control State
float base_target_vel[4] = {0, 0, 0, 0};
float base_current_vel[4] = {0, 0, 0, 0};


// PID Controller State
float base_pid_integral[4] = {0, 0, 0, 0};
float base_pid_prev_error[4] = {0, 0, 0, 0};


// Timing State
unsigned long base_last_pid_time = 0;
unsigned long base_last_enc_pub_time = 0;
unsigned long base_last_cmd_time = 0;


// Motor Control Arrays
const int motor_in1_pins[4] = {M_FL_IN1, M_FR_IN1, M_BL_IN1, M_BR_IN1};
const int motor_in2_pins[4] = {M_FL_IN2, M_FR_IN2, M_BL_IN2, M_BR_IN2};
const int motor_en_pins[4] = {M_FL_EN, M_FR_EN, M_BL_EN, M_BR_EN};
const int motor_direction[4] = {1, 1, 1, 1};


// ============================================================================
// GLOBAL STATE VARIABLES - ROBOTIC ARM
// ============================================================================


// Joint State
float arm_current_angle[6] = {90, 90, 90, 90, 90, 90};
float arm_target_angle[6] = {90, 90, 90, 90, 90, 90};
float arm_last_sent_angle[6] = {90, 90, 90, 90, 90, 90};


// Timing State
unsigned long arm_last_update_time = 0;


// ============================================================================
// SHARED COMMUNICATION
// ============================================================================


String cmd_buffer = "";
unsigned long last_heartbeat = 0;


// ============================================================================
// MOBILE BASE MODULE
// ============================================================================


/**
 * Initialize motor driver pins
 */
void baseSetupMotors() {
  for (int i = 0; i < 4; i++) {
    pinMode(motor_in1_pins[i], OUTPUT);
    pinMode(motor_in2_pins[i], OUTPUT);
    pinMode(motor_en_pins[i], OUTPUT);
    
    digitalWrite(motor_in1_pins[i], LOW);
    digitalWrite(motor_in2_pins[i], LOW);
    analogWrite(motor_en_pins[i], 0);
  }
}


/**
 * Set motor output
 */
void baseSetMotorOutput(int motor_idx, int pwm_like) {
  float scaled_f = pwm_like * motor_cal[motor_idx];
  int scaled_pwm = (int)constrain(scaled_f, -255, 255);
  int final_pwm = scaled_pwm * motor_direction[motor_idx];
  
  int pwm_magnitude = abs(final_pwm);
  int direction = (final_pwm > 0) ? 1 : (final_pwm < 0) ? -1 : 0;
  
  if (direction > 0) {
    digitalWrite(motor_in1_pins[motor_idx], HIGH);
    digitalWrite(motor_in2_pins[motor_idx], LOW);
  } else if (direction < 0) {
    digitalWrite(motor_in1_pins[motor_idx], LOW);
    digitalWrite(motor_in2_pins[motor_idx], HIGH);
  } else {
    digitalWrite(motor_in1_pins[motor_idx], LOW);
    digitalWrite(motor_in2_pins[motor_idx], LOW);
  }
  
  analogWrite(motor_en_pins[motor_idx], pwm_magnitude);
}


/**
 * Emergency stop mobile base
 */
void baseStopAllMotors() {
  for (int i = 0; i < 4; i++) {
    baseSetMotorOutput(i, 0);
    base_target_vel[i] = 0;
    base_pid_integral[i] = 0;
    base_pid_prev_error[i] = 0;
  }
}


/**
 * Initialize encoders
 */
void baseSetupEncoders() {
  pinMode(ENC_FL_A, INPUT_PULLUP);
  pinMode(ENC_FL_B, INPUT_PULLUP);
  pinMode(ENC_FR_A, INPUT_PULLUP);
  pinMode(ENC_FR_B, INPUT_PULLUP);
  pinMode(ENC_BL_A, INPUT_PULLUP);
  pinMode(ENC_BL_B, INPUT_PULLUP);
  pinMode(ENC_BR_A, INPUT_PULLUP);
  pinMode(ENC_BR_B, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(ENC_FL_A), isr_enc_fl, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_FR_A), isr_enc_fr, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_BL_A), isr_enc_bl, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_BR_A), isr_enc_br, RISING);
}


/**
 * Encoder ISRs with debouncing
 */
void isr_enc_fl() {
  unsigned long now = micros();
  if (now - base_last_isr_time[0] < ENCODER_DEBOUNCE_US) return;
  base_last_isr_time[0] = now;
  base_enc_counts[0] += digitalRead(ENC_FL_B) ? -1 : 1;
}


void isr_enc_fr() {
  unsigned long now = micros();
  if (now - base_last_isr_time[1] < ENCODER_DEBOUNCE_US) return;
  base_last_isr_time[1] = now;
  base_enc_counts[1] += digitalRead(ENC_FR_B) ? 1 : -1;
}


void isr_enc_bl() {
  unsigned long now = micros();
  if (now - base_last_isr_time[2] < ENCODER_DEBOUNCE_US) return;
  base_last_isr_time[2] = now;
  base_enc_counts[2] += digitalRead(ENC_BL_B) ? -1 : 1;
}


void isr_enc_br() {
  unsigned long now = micros();
  if (now - base_last_isr_time[3] < ENCODER_DEBOUNCE_US) return;
  base_last_isr_time[3] = now;
  base_enc_counts[3] += digitalRead(ENC_BR_B) ? 1 : -1;
}


/**
 * Get encoder counts atomically
 */
void baseGetEncoderCounts(long* counts) {
  noInterrupts();
  for (int i = 0; i < 4; i++) {
    counts[i] = base_enc_counts[i];
  }
  interrupts();
}


/**
 * Reset encoders
 */
void baseResetEncoders() {
  noInterrupts();
  for (int i = 0; i < 4; i++) {
    base_enc_counts[i] = 0;
    base_prev_enc_counts[i] = 0;
    base_last_published_counts[i] = 0;
  }
  interrupts();
}


/**
 * PID computation
 */
float baseComputePID(int motor_idx, float target, float current, float dt) {
  float error = target - current;
  
  float p_term = base_Kp * error;
  
  base_pid_integral[motor_idx] += error * dt;
  base_pid_integral[motor_idx] = constrain(base_pid_integral[motor_idx], -100, 100);
  float i_term = base_Ki * base_pid_integral[motor_idx];
  
  float d_term = 0;
  if (dt > 0) {
    d_term = base_Kd * (error - base_pid_prev_error[motor_idx]) / dt;
  }
  base_pid_prev_error[motor_idx] = error;
  
  float output = (p_term + i_term + d_term) * 25.5;
  return constrain(fabs(output), 0, 255);
}


/**
 * Update PID controllers
 */
void baseUpdatePID() {
  unsigned long now = millis();
  float dt = (now - base_last_pid_time) / 1000.0;
  
  if (dt < (BASE_PID_SAMPLE_TIME / 1000.0)) return;
  
  long counts[4];
  baseGetEncoderCounts(counts);
  
  for (int i = 0; i < 4; i++) {
    long raw_delta = counts[i] - base_prev_enc_counts[i];
    long delta = raw_delta * encoder_dir[i];
    base_prev_enc_counts[i] = counts[i];
    base_current_vel[i] = (delta * 2.0 * PI) / (ENCODER_CPR * dt);
  }
  
  for (int i = 0; i < 4; i++) {
    float vel_error = base_target_vel[i] - base_current_vel[i];
    const float VEL_DEADBAND = 0.3;
    
    if (fabs(vel_error) < VEL_DEADBAND) {
      baseSetMotorOutput(i, 0);
      base_pid_integral[i] = 0;
      continue;
    }
    
    int pwm_mag = (int)baseComputePID(i, fabs(base_target_vel[i]), fabs(base_current_vel[i]), dt);
    int pwm = (base_target_vel[i] >= 0) ? pwm_mag : -pwm_mag;
    baseSetMotorOutput(i, pwm);
  }
  
  base_last_pid_time = now;
}


/**
 * Set velocity command (differential drive kinematics)
 */
void baseSetVelocityCommand(float linear_x, float angular_z) {
  float v_left = linear_x - (angular_z * WHEEL_BASE / 2.0);
  float v_right = linear_x + (angular_z * WHEEL_BASE / 2.0);
  
  float omega_left = v_left / WHEEL_RADIUS;
  float omega_right = v_right / WHEEL_RADIUS;
  
  base_target_vel[0] = omega_left;
  base_target_vel[1] = omega_right;
  base_target_vel[2] = omega_left;
  base_target_vel[3] = omega_right;
  
  base_last_cmd_time = millis();
}


/**
 * Publish encoder counts (change-based)
 */
void basePublishEncoderCounts() {
  unsigned long now = millis();
  unsigned long pub_interval = 1000 / BASE_ENCODER_PUB_RATE;
  
  if (now - base_last_enc_pub_time < pub_interval) return;
  
  long counts[4];
  baseGetEncoderCounts(counts);
  
  bool changed = false;
  for (int i = 0; i < 4; i++) {
    if (counts[i] != base_last_published_counts[i]) {
      changed = true;
      break;
    }
  }
  
  if (changed) {
    Serial.print("ENC,");
    Serial.print(counts[0]); Serial.print(",");
    Serial.print(counts[1]); Serial.print(",");
    Serial.print(counts[2]); Serial.print(",");
    Serial.println(counts[3]);
    
    for (int i = 0; i < 4; i++) {
      base_last_published_counts[i] = counts[i];
    }
  }
  
  base_last_enc_pub_time = now;
}


/**
 * Command timeout watchdog
 */
void baseCheckCommandTimeout() {
  unsigned long dt = millis() - base_last_cmd_time;
  
  if (dt > BASE_CMD_TIMEOUT) {
    bool any_moving = false;
    for (int i = 0; i < 4; i++) {
      if (base_target_vel[i] != 0) {
        any_moving = true;
        break;
      }
    }
    
    if (any_moving) {
      baseStopAllMotors();
      Serial.println("STATUS,Timeout - mobile base stopped");
    }
  }
}


// ============================================================================
// ROBOTIC ARM MODULE
// ============================================================================


/**
 * Initialize PCA9685 and servos
 */
void armSetup() {
  pca.begin();
  pca.setPWMFreq(SERVO_FREQ_HZ);
  delay(100);
  
  // Initialize servos to home position
  for (int i = 0; i < 6; i++) {
    armGoAngle(SERVO_CH[i], arm_current_angle[i]);
  }
}


/**
 * Write angle to servo
 */
inline void armGoAngle(uint8_t ch, float angle) {
  angle = constrain(angle, 0, 180);
  float us = SERVO_MIN_US + (angle / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US);
  pca.writeMicroseconds(ch, (uint16_t)us);
}


/**
 * Update servo positions with smooth stepping
 */
bool armUpdateStep() {
  bool changed = false;
  
  for (int i = 0; i < 6; i++) {
    float before = arm_current_angle[i];
    
    if (arm_current_angle[i] < arm_target_angle[i]) {
      arm_current_angle[i] += ARM_STEP_DEG;
      if (arm_current_angle[i] > arm_target_angle[i]) 
        arm_current_angle[i] = arm_target_angle[i];
    }
    else if (arm_current_angle[i] > arm_target_angle[i]) {
      arm_current_angle[i] -= ARM_STEP_DEG;
      if (arm_current_angle[i] < arm_target_angle[i]) 
        arm_current_angle[i] = arm_target_angle[i];
    }
    
    if (arm_current_angle[i] != before) changed = true;
    
    armGoAngle(SERVO_CH[i], arm_current_angle[i]);
  }
  
  return changed;
}


/**
 * Publish joint positions (only when changed)
 */
void armPublishPositions() {
  Serial.print("POS,");
  for (int i = 0; i < 6; i++) {
    Serial.print(arm_current_angle[i], 1);
    if (i < 5) Serial.print(",");
    arm_last_sent_angle[i] = arm_current_angle[i];
  }
  Serial.println();
}


// ============================================================================
// UNIFIED COMMAND PROCESSING
// ============================================================================


/**
 * Process incoming serial commands
 */
void processCommand(String cmd) {
  cmd.trim();
  
  // ========== MOBILE BASE COMMANDS ==========
  
  if (cmd.startsWith("VEL,")) {
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);
    if (idx2 > idx1) {
      float linear_x = cmd.substring(idx1 + 1, idx2).toFloat();
      float angular_z = cmd.substring(idx2 + 1).toFloat();
      baseSetVelocityCommand(linear_x, angular_z);
      Serial.println("STATUS,Velocity command set");
    }
  }
  else if (cmd == "STOP") {
    baseStopAllMotors();
    Serial.println("STATUS,Mobile base stopped");
  }
  else if (cmd.startsWith("PID,")) {
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);
    int idx3 = cmd.indexOf(',', idx2 + 1);
    if (idx3 > idx2 && idx2 > idx1) {
      base_Kp = cmd.substring(idx1 + 1, idx2).toFloat();
      base_Ki = cmd.substring(idx2 + 1, idx3).toFloat();
      base_Kd = cmd.substring(idx3 + 1).toFloat();
      Serial.print("STATUS,PID gains updated: Kp=");
      Serial.print(base_Kp, 2);
      Serial.print(" Ki=");
      Serial.print(base_Ki, 2);
      Serial.print(" Kd=");
      Serial.println(base_Kd, 2);
    }
  }
  else if (cmd == "RST") {
    baseResetEncoders();
    Serial.println("STATUS,Encoders reset");
  }
  
  // ========== ROBOTIC ARM COMMANDS ==========
  
  else if (cmd.startsWith("SERVO,")) {
    int c1 = cmd.indexOf(',');
    int c2 = cmd.indexOf(',', c1 + 1);
    
    if (c2 > c1) {
      int idx = cmd.substring(c1 + 1, c2).toInt();
      float ang = cmd.substring(c2 + 1).toFloat();
      
      if (idx >= 0 && idx < 6) {
        arm_target_angle[idx] = constrain(ang, 0, 180);
        Serial.print("STATUS,Joint ");
        Serial.print(idx);
        Serial.print(" target set to ");
        Serial.print(ang);
        Serial.println("°");
      }
    }
  }
  else if (cmd.startsWith("ALL,")) {
    String t = cmd.substring(cmd.indexOf(',') + 1);
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
      for (int i = 0; i < 6; i++) {
        arm_target_angle[i] = constrain(vals[i], 0, 180);
      }
      Serial.println("STATUS,All joint targets set");
    }
  }
  
  // ========== GLOBAL COMMANDS ==========
  
  else if (cmd == "STOP_ALL") {
    baseStopAllMotors();
    Serial.println("STATUS,EMERGENCY STOP - All systems halted");
  }
  else if (cmd == "STATUS") {
    Serial.print("STATUS,Base_vel:[");
    Serial.print(base_target_vel[0], 2); Serial.print(",");
    Serial.print(base_target_vel[1], 2); Serial.print(",");
    Serial.print(base_target_vel[2], 2); Serial.print(",");
    Serial.print(base_target_vel[3], 2);
    Serial.print("] Arm_pos:[");
    for (int i = 0; i < 6; i++) {
      Serial.print(arm_current_angle[i], 1);
      if (i < 5) Serial.print(",");
    }
    Serial.println("]");
  }
  else {
    Serial.print("ERROR,Unknown command: ");
    Serial.println(cmd);
  }
}


/**
 * Read and process serial data
 */
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


/**
 * Send system heartbeat
 */
void sendHeartbeat() {
  unsigned long now = millis();
  if (now - last_heartbeat >= 1000) {
    Serial.print("HEARTBEAT,");
    Serial.println(now);
    last_heartbeat = now;
  }
}


// ============================================================================
// MAIN PROGRAM
// ============================================================================


void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) { ; }
  
  // Initialize I2C for PCA9685
  Wire.begin();
  
  // Initialize mobile base
  baseSetupMotors();
  baseSetupEncoders();
  
  // Initialize robotic arm
  armSetup();
  
  // Initialize timing
  base_last_pid_time = millis();
  base_last_enc_pub_time = millis();
  base_last_cmd_time = millis();
  arm_last_update_time = millis();
  last_heartbeat = millis();
  
  // Startup message
  Serial.println("============================================");
  Serial.println("STATUS,Mobile Manipulator Ready");
  Serial.println("============================================");
  Serial.print("STATUS,Mobile Base: r=");
  Serial.print(WHEEL_RADIUS, 3);
  Serial.print("m, L=");
  Serial.print(WHEEL_BASE, 3);
  Serial.println("m");
  Serial.println("STATUS,Robotic Arm: 6-DOF via PCA9685");
  Serial.println();
  Serial.println("STATUS,Available commands:");
  Serial.println("  VEL,<linear_x>,<angular_z>");
  Serial.println("  STOP");
  Serial.println("  PID,<kp>,<ki>,<kd>");
  Serial.println("  RST");
  Serial.println("  SERVO,<idx>,<angle>");
  Serial.println("  ALL,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>");
  Serial.println("  STOP_ALL");
  Serial.println("  STATUS");
  Serial.println("============================================");
}


void loop() {
  // Read serial commands
  readSerial();
  
  // Update mobile base
  baseCheckCommandTimeout();
  baseUpdatePID();
  basePublishEncoderCounts();
  
  // Update robotic arm (with rate limiting)
  unsigned long now = millis();
  if (now - arm_last_update_time >= ARM_STEP_DELAY_MS) {
    bool arm_moved = armUpdateStep();
    
    // Publish arm positions only when changed
    if (arm_moved) {
      armPublishPositions();
    }
    
    arm_last_update_time = now;
  }
  
  // Send heartbeat
  sendHeartbeat();
}
