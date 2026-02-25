/*
 * ============================================================================
 * MOBILE BASE CONTROLLER - TESTING & DEBUG VERSION
 * ============================================================================
 * 
 * Extended version with comprehensive diagnostic and testing tools
 * 
 * ADDITIONAL DEBUG FEATURES:
 * - Test mode with simulated encoders (no hardware required)
 * - Hardware pin state checking (HWCHK)
 * - Per-motor manual control (MOT)
 * - Encoder direction verification (ENCHECK)
 * - Motor locking on unexpected direction flips
 * - Configurable encoder debouncing (DEBOUNCE)
 * - Per-motor calibration tuning (CAL)
 * - Detailed hardware diagnostics
 * 
 * EXTENDED SERIAL PROTOCOL:
 *   ROS -> Arduino (Production commands plus):
 *     TEST,ON/OFF                 - Enable/disable simulated encoders
 *     MOT,<idx>,<pwm>            - Direct motor control (-255 to 255)
 *     HWCHK                      - Print hardware pin states
 *     ENCHECK                    - Run encoder direction test
 *     DEBOUNCE,<us>              - Set debounce window (microseconds)
 *     CAL,<s0>,<s1>,<s2>,<s3>   - Set motor calibration factors
 *     UNLOCK,<idx|ALL>           - Unlock motor after flip detection
 * 
 *   Arduino -> ROS (Production messages plus):
 *     HW,<type>,<data>           - Hardware diagnostic messages
 *     TEST,<message>             - Test mode messages
 * 
 * USE CASES:
 * - Verify wiring without physical robot movement (TEST,ON)
 * - Diagnose encoder polarity issues (ENCHECK)
 * - Test individual motors (MOT command)
 * - Tune motor calibration factors for straight-line motion
 * - Debug electrical noise issues (DEBOUNCE tuning)
 * 
 * AUTHOR: [Your Name]
 * VERSION: 1.0 (Debug)
 * DATE: December 2025
 * ============================================================================
 */

// ============================================================================
// CONFIGURATION PARAMETERS
// ============================================================================

// Serial Communication
#define SERIAL_BAUD 115200

// Robot Physical Parameters
#define WHEEL_RADIUS     0.075    // meters
#define WHEEL_BASE       0.35     // meters
#define ENCODER_CPR      360      // Counts per revolution

// Control Loop Timing
#define PID_SAMPLE_TIME  20       // milliseconds
#define ENCODER_PUB_RATE 50       // Hz
#define CMD_TIMEOUT      2000     // milliseconds

// PID Controller Gains
float Kp = 2.0;
float Ki = 0.5;
float Kd = 0.1;

// Encoder Debounce (tunable for noise debugging)
unsigned long encoder_debounce_us = 200;  // microseconds

// Motor Calibration Factors
float motor_cal[4] = {0.902, 0.750, 1.00, 0.80};  // FL, FR, BL, BR

// ============================================================================
// HARDWARE PIN DEFINITIONS
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
#define ENC_FL_A  18
#define ENC_FL_B  31
#define ENC_FR_A  19
#define ENC_FR_B  33
#define ENC_BL_A  20
#define ENC_BL_B  35
#define ENC_BR_A  21
#define ENC_BR_B  37

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

// Encoder State
volatile long enc_counts[4] = {0, 0, 0, 0};
volatile unsigned long last_isr_time[4] = {0, 0, 0, 0};
long prev_enc_counts[4] = {0, 0, 0, 0};
long last_published_counts[4] = {0, 0, 0, 0};
const int encoder_dir[4] = {1, -1, 1, -1};

// Velocity Control State
float target_vel[4] = {0, 0, 0, 0};
float current_vel[4] = {0, 0, 0, 0};

// PID Controller State
float pid_integral[4] = {0, 0, 0, 0};
float pid_prev_error[4] = {0, 0, 0, 0};

// Motor State Tracking (for debugging)
int motor_state[4] = {0, 0, 0, 0};        // -1=reverse, 0=stop, 1=forward
int prev_motor_state[4] = {0, 0, 0, 0};
bool motor_locked[4] = {false, false, false, false};  // Lock on direction flip

// Timing State
unsigned long last_pid_time = 0;
unsigned long last_enc_pub_time = 0;
unsigned long last_cmd_time = 0;
unsigned long last_test_update_time = 0;

// Serial Command Buffer
String cmd_buffer = "";

// Motor Control Arrays
const int motor_in1_pins[4] = {M_FL_IN1, M_FR_IN1, M_BL_IN1, M_BR_IN1};
const int motor_in2_pins[4] = {M_FL_IN2, M_FR_IN2, M_BL_IN2, M_BR_IN2};
const int motor_en_pins[4] = {M_FL_EN, M_FR_EN, M_BL_EN, M_BR_EN};
const int motor_direction[4] = {1, 1, 1, 1};

// TEST MODE - Encoder Simulation
bool test_mode = false;
float sim_enc_accumulator[4] = {0.0, 0.0, 0.0, 0.0};

// Manual Motor Control Tracking
bool manual_active[4] = {false, false, false, false};

// ============================================================================
// MOTOR CONTROL MODULE
// ============================================================================

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

/**
 * Set motor output with direction flip detection
 * Locks motor if unexpected direction change detected (for diagnosis)
 */
void setMotorOutput(int motor_idx, int pwm_like) {
  // Respect motor lock
  if (motor_locked[motor_idx] && pwm_like != 0) {
    Serial.print("HW,LOCKED,M");
    Serial.print(motor_idx);
    Serial.println(",Command ignored");
    return;
  }
  
  // Apply calibration
  float scaled_f = pwm_like * motor_cal[motor_idx];
  int scaled_pwm = (int)constrain(scaled_f, -255, 255);
  int final_pwm = scaled_pwm * motor_direction[motor_idx];
  
  int pwm_magnitude = abs(final_pwm);
  int direction = (final_pwm > 0) ? 1 : (final_pwm < 0) ? -1 : 0;
  
  // Set direction
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
  
  // Detect direction flip (forward <-> reverse transition)
  if (motor_state[motor_idx] != prev_motor_state[motor_idx]) {
    bool is_flip = (prev_motor_state[motor_idx] == 1 && motor_state[motor_idx] == -1) ||
                   (prev_motor_state[motor_idx] == -1 && motor_state[motor_idx] == 1);
    
    if (is_flip) {
      long counts[4];
      getEncoderCounts(counts);
      
      Serial.print("HW,DIR_FLIP,M");
      Serial.print(motor_idx);
      Serial.print(",");
      Serial.print(prev_motor_state[motor_idx]);
      Serial.print("->");
      Serial.print(motor_state[motor_idx]);
      Serial.print(",ENC=[");
      Serial.print(counts[0]); Serial.print(",");
      Serial.print(counts[1]); Serial.print(",");
      Serial.print(counts[2]); Serial.print(",");
      Serial.print(counts[3]); Serial.println("]");
      
      // Lock motor if not in test mode
      if (!test_mode) {
        analogWrite(motor_en_pins[motor_idx], 0);
        digitalWrite(motor_in1_pins[motor_idx], LOW);
        digitalWrite(motor_in2_pins[motor_idx], LOW);
        motor_locked[motor_idx] = true;
        motor_state[motor_idx] = 0;
        Serial.print("HW,MOTOR_LOCKED,M");
        Serial.println(motor_idx);
      }
    }
    
    prev_motor_state[motor_idx] = motor_state[motor_idx];
  }
}

void stopAllMotors() {
  for (int i = 0; i < 4; i++) {
    setMotorOutput(i, 0);
    target_vel[i] = 0;
    pid_integral[i] = 0;
    pid_prev_error[i] = 0;
    manual_active[i] = false;
  }
}

/**
 * DEBUG: Print comprehensive hardware status
 */
void printHardwareStatus() {
  Serial.println("HW,STATUS,=== Pin Mapping & Levels ===");
  
  for (int i = 0; i < 4; i++) {
    Serial.print("HW,MOTOR,M");
    Serial.print(i);
    Serial.print(",IN1=D");
    Serial.print(motor_in1_pins[i]);
    Serial.print(":");
    Serial.print(digitalRead(motor_in1_pins[i]));
    Serial.print(",IN2=D");
    Serial.print(motor_in2_pins[i]);
    Serial.print(":");
    Serial.print(digitalRead(motor_in2_pins[i]));
    Serial.print(",EN=D");
    Serial.print(motor_en_pins[i]);
    Serial.print(",State=");
    Serial.print(motor_state[i]);
    Serial.print(",Locked=");
    Serial.println(motor_locked[i]);
  }
  
  Serial.print("HW,ENC,FL:[");
  Serial.print(digitalRead(ENC_FL_A));
  Serial.print(",");
  Serial.print(digitalRead(ENC_FL_B));
  Serial.print("], FR:[");
  Serial.print(digitalRead(ENC_FR_A));
  Serial.print(",");
  Serial.print(digitalRead(ENC_FR_B));
  Serial.print("], BL:[");
  Serial.print(digitalRead(ENC_BL_A));
  Serial.print(",");
  Serial.print(digitalRead(ENC_BL_B));
  Serial.print("], BR:[");
  Serial.print(digitalRead(ENC_BR_A));
  Serial.print(",");
  Serial.print(digitalRead(ENC_BR_B));
  Serial.println("]");
  
  long counts[4];
  getEncoderCounts(counts);
  Serial.print("HW,ENC_COUNTS,[");
  Serial.print(counts[0]); Serial.print(",");
  Serial.print(counts[1]); Serial.print(",");
  Serial.print(counts[2]); Serial.print(",");
  Serial.print(counts[3]); Serial.println("]");
}

// ============================================================================
// ENCODER MODULE
// ============================================================================

void setupEncoders() {
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

void isr_enc_fl() {
  unsigned long now = micros();
  if (now - last_isr_time[0] < encoder_debounce_us) return;
  last_isr_time[0] = now;
  enc_counts[0] += digitalRead(ENC_FL_B) ? -1 : 1;
}

void isr_enc_fr() {
  unsigned long now = micros();
  if (now - last_isr_time[1] < encoder_debounce_us) return;
  last_isr_time[1] = now;
  enc_counts[1] += digitalRead(ENC_FR_B) ? 1 : -1;
}

void isr_enc_bl() {
  unsigned long now = micros();
  if (now - last_isr_time[2] < encoder_debounce_us) return;
  last_isr_time[2] = now;
  enc_counts[2] += digitalRead(ENC_BL_B) ? -1 : 1;
}

void isr_enc_br() {
  unsigned long now = micros();
  if (now - last_isr_time[3] < encoder_debounce_us) return;
  last_isr_time[3] = now;
  enc_counts[3] += digitalRead(ENC_BR_B) ? 1 : -1;
}

void getEncoderCounts(long* counts) {
  noInterrupts();
  for (int i = 0; i < 4; i++) {
    counts[i] = enc_counts[i];
  }
  interrupts();
}

void resetEncoders() {
  noInterrupts();
  for (int i = 0; i < 4; i++) {
    enc_counts[i] = 0;
    prev_enc_counts[i] = 0;
    last_published_counts[i] = 0;
  }
  interrupts();
  
  for (int i = 0; i < 4; i++) {
    sim_enc_accumulator[i] = 0.0;
  }
}

// ============================================================================
// TEST MODE - SIMULATED ENCODERS
// ============================================================================

/**
 * Update simulated encoder counts based on target velocities
 * Allows testing control logic without physical hardware
 */
void updateSimulatedEncoders() {
  if (!test_mode) return;
  
  unsigned long now = millis();
  float dt = (now - last_test_update_time) / 1000.0;
  
  if (dt <= 0 || dt > 0.1) {
    last_test_update_time = now;
    return;
  }
  
  for (int i = 0; i < 4; i++) {
    // Calculate expected ticks: ticks = (ω * dt * CPR) / (2π)
    float delta_ticks = (target_vel[i] * dt * ENCODER_CPR) / (2.0 * PI);
    
    sim_enc_accumulator[i] += delta_ticks;
    long whole_ticks = (long)sim_enc_accumulator[i];
    
    if (whole_ticks != 0) {
      noInterrupts();
      enc_counts[i] += whole_ticks;
      interrupts();
      sim_enc_accumulator[i] -= whole_ticks;
    }
  }
  
  last_test_update_time = now;
}

// ============================================================================
// PID CONTROLLER MODULE
// ============================================================================

float computePID(int motor_idx, float target, float current, float dt) {
  float error = target - current;
  
  float p_term = Kp * error;
  
  pid_integral[motor_idx] += error * dt;
  pid_integral[motor_idx] = constrain(pid_integral[motor_idx], -100, 100);
  float i_term = Ki * pid_integral[motor_idx];
  
  float d_term = 0;
  if (dt > 0) {
    d_term = Kd * (error - pid_prev_error[motor_idx]) / dt;
  }
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
    const float VEL_DEADBAND = 0.3;
    
    if (fabs(vel_error) < VEL_DEADBAND) {
      setMotorOutput(i, 0);
      pid_integral[i] = 0;
      continue;
    }
    
    int pwm_mag = (int)computePID(i, fabs(target_vel[i]), fabs(current_vel[i]), dt);
    int pwm = (target_vel[i] >= 0) ? pwm_mag : -pwm_mag;
    setMotorOutput(i, pwm);
  }
  
  last_pid_time = now;
}

// ============================================================================
// DIFFERENTIAL DRIVE KINEMATICS
// ============================================================================

void setVelocityCommand(float linear_x, float angular_z) {
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

// ============================================================================
// SERIAL COMMUNICATION MODULE
// ============================================================================

void processCommand(String cmd) {
  cmd.trim();
  
  // Production commands
  if (cmd.startsWith("VEL,")) {
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);
    if (idx2 > idx1) {
      float linear_x = cmd.substring(idx1 + 1, idx2).toFloat();
      float angular_z = cmd.substring(idx2 + 1).toFloat();
      setVelocityCommand(linear_x, angular_z);
    }
  }
  else if (cmd == "STOP") {
    stopAllMotors();
    Serial.println("STATUS,Stopped");
  }
  else if (cmd.startsWith("PID,")) {
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);
    int idx3 = cmd.indexOf(',', idx2 + 1);
    if (idx3 > idx2 && idx2 > idx1) {
      Kp = cmd.substring(idx1 + 1, idx2).toFloat();
      Ki = cmd.substring(idx2 + 1, idx3).toFloat();
      Kd = cmd.substring(idx3 + 1).toFloat();
      Serial.print("STATUS,PID: Kp=");
      Serial.print(Kp, 2);
      Serial.print(" Ki=");
      Serial.print(Ki, 2);
      Serial.print(" Kd=");
      Serial.println(Kd, 2);
    }
  }
  else if (cmd == "RST") {
    resetEncoders();
    Serial.println("STATUS,Encoders reset");
  }
  else if (cmd == "STATUS") {
    Serial.print("STATUS,Mode:");
    Serial.print(test_mode ? "TEST" : "NORMAL");
    Serial.print(",Vel:[");
    Serial.print(target_vel[0], 2); Serial.print(",");
    Serial.print(target_vel[1], 2); Serial.print(",");
    Serial.print(target_vel[2], 2); Serial.print(",");
    Serial.print(target_vel[3], 2); Serial.println("]");
  }
  
  // ========== DEBUG COMMANDS ==========
  
  else if (cmd == "TEST,ON") {
    test_mode = true;
    last_test_update_time = millis();
    resetEncoders();
    Serial.println("TEST,Mode ENABLED - simulated encoders");
  }
  else if (cmd == "TEST,OFF") {
    test_mode = false;
    resetEncoders();
    Serial.println("TEST,Mode DISABLED - real encoders");
  }
  else if (cmd == "HWCHK") {
    printHardwareStatus();
  }
  else if (cmd.startsWith("MOT,")) {
    // Direct motor control: MOT,<idx>,<pwm>
    int i1 = cmd.indexOf(',');
    int i2 = cmd.indexOf(',', i1 + 1);
    if (i2 > i1) {
      int m_idx = cmd.substring(i1 + 1, i2).toInt();
      int pwm = cmd.substring(i2 + 1).toInt();
      if (m_idx >= 0 && m_idx < 4) {
        setMotorOutput(m_idx, pwm);
        manual_active[m_idx] = true;
        last_cmd_time = millis();
        Serial.print("TEST,MOT,M");
        Serial.print(m_idx);
        Serial.print("=");
        Serial.println(pwm);
      }
    }
  }
  else if (cmd.startsWith("DEBOUNCE,")) {
    int idx = cmd.indexOf(',');
    if (idx > 0) {
      unsigned long new_val = cmd.substring(idx + 1).toInt();
      if (new_val <= 10000) {
        encoder_debounce_us = new_val;
        Serial.print("TEST,Debounce=");
        Serial.print(encoder_debounce_us);
        Serial.println("us");
      }
    }
  }
  else if (cmd.startsWith("CAL,")) {
    // Motor calibration: CAL,<s0>,<s1>,<s2>,<s3>
    int i1 = cmd.indexOf(',');
    int i2 = cmd.indexOf(',', i1 + 1);
    int i3 = cmd.indexOf(',', i2 + 1);
    int i4 = cmd.indexOf(',', i3 + 1);
    if (i4 > i3 && i3 > i2 && i2 > i1) {
      motor_cal[0] = cmd.substring(i1 + 1, i2).toFloat();
      motor_cal[1] = cmd.substring(i2 + 1, i3).toFloat();
      motor_cal[2] = cmd.substring(i3 + 1, i4).toFloat();
      motor_cal[3] = cmd.substring(i4 + 1).toFloat();
      Serial.print("TEST,CAL=[");
      Serial.print(motor_cal[0], 3); Serial.print(",");
      Serial.print(motor_cal[1], 3); Serial.print(",");
      Serial.print(motor_cal[2], 3); Serial.print(",");
      Serial.print(motor_cal[3], 3); Serial.println("]");
    }
  }
  else if (cmd == "ENCHECK") {
    // Encoder direction verification test
    Serial.println("TEST,ENCHECK,Starting...");
    
    for (int m = 0; m < 4; m++) {
      if (motor_locked[m]) {
        Serial.print("TEST,ENCHECK,M");
        Serial.print(m);
        Serial.println(",LOCKED");
        continue;
      }
      
      // Forward pulse
      long before[4];
      getEncoderCounts(before);
      setMotorOutput(m, 150);
      delay(500);
      setMotorOutput(m, 0);
      long after[4];
      getEncoderCounts(after);
      long delta_fw = after[m] - before[m];
      
      delay(200);
      
      // Reverse pulse
      getEncoderCounts(before);
      setMotorOutput(m, -150);
      delay(500);
      setMotorOutput(m, 0);
      getEncoderCounts(after);
      long delta_rev = after[m] - before[m];
      
      Serial.print("TEST,ENCHECK,M");
      Serial.print(m);
      Serial.print(",FW=");
      Serial.print(delta_fw);
      Serial.print(",REV=");
      Serial.println(delta_rev);
      
      delay(200);
    }
    
    Serial.println("TEST,ENCHECK,Complete");
  }
  else if (cmd.startsWith("UNLOCK,")) {
    int idx = cmd.indexOf(',');
    String arg = cmd.substring(idx + 1);
    if (arg == "ALL") {
      for (int i = 0; i < 4; i++) motor_locked[i] = false;
      Serial.println("TEST,Unlocked ALL motors");
    } else {
      int m = arg.toInt();
      if (m >= 0 && m < 4) {
        motor_locked[m] = false;
        Serial.print("TEST,Unlocked M");
        Serial.println(m);
      }
    }
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

void publishEncoderCounts() {
  unsigned long now = millis();
  unsigned long pub_interval = 1000 / ENCODER_PUB_RATE;
  
  if (now - last_enc_pub_time < pub_interval) return;
  
  long counts[4];
  getEncoderCounts(counts);
  
  bool changed = false;
  for (int i = 0; i < 4; i++) {
    if (counts[i] != last_published_counts[i]) {
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
      last_published_counts[i] = counts[i];
    }
  }
  
  last_enc_pub_time = now;
}

void checkCommandTimeout() {
  unsigned long dt = millis() - last_cmd_time;
  
  if (dt > CMD_TIMEOUT) {
    bool any_active = false;
    for (int i = 0; i < 4; i++) {
      if (target_vel[i] != 0 || manual_active[i]) {
        any_active = true;
        break;
      }
    }
    
    if (any_active) {
      stopAllMotors();
      Serial.print("STATUS,Timeout (");
      Serial.print(dt);
      Serial.println("ms) - stopped");
    }
  }
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { ; }
  
  setupMotors();
  setupEncoders();
  
  last_pid_time = millis();
  last_enc_pub_time = millis();
  last_cmd_time = millis();
  last_test_update_time = millis();
  
  Serial.println("========================================");
  Serial.println("STATUS,Mobile Base Controller [DEBUG]");
  Serial.println("========================================");
  Serial.print("STATUS,Wheel: r=");
  Serial.print(WHEEL_RADIUS, 3);
  Serial.print("m, L=");
  Serial.print(WHEEL_BASE, 3);
  Serial.println("m");
  Serial.print("STATUS,PID: Kp=");
  Serial.print(Kp, 2);
  Serial.print(" Ki=");
  Serial.print(Ki, 2);
  Serial.print(" Kd=");
  Serial.println(Kd, 2);
  Serial.println();
  Serial.println("STATUS,Available commands:");
  Serial.println("  PRODUCTION:");
  Serial.println("    VEL,<x>,<z>        - Velocity command");
  Serial.println("    STOP               - Emergency stop");
  Serial.println("    PID,<kp>,<ki>,<kd> - Tune PID");
  Serial.println("    RST                - Reset encoders");
  Serial.println("    STATUS             - Query status");
  Serial.println("  DEBUG:");
  Serial.println("    TEST,ON/OFF        - Simulate encoders");
  Serial.println("    MOT,<idx>,<pwm>    - Direct motor test");
  Serial.println("    HWCHK              - Pin state check");
  Serial.println("    ENCHECK            - Encoder direction test");
  Serial.println("    DEBOUNCE,<us>      - Set debounce window");
  Serial.println("    CAL,<s0...s3>      - Motor calibration");
  Serial.println("    UNLOCK,<idx|ALL>   - Unlock motors");
  Serial.println("========================================");
  
  printHardwareStatus();
}

void loop() {
  readSerial();
  checkCommandTimeout();
  
  // Test mode updates
  if (test_mode) {
    updateSimulatedEncoders();
  }
  
  // PID control (skip in test mode)
  if (!test_mode) {
    updatePID();
  }
  
  publishEncoderCounts();
}
