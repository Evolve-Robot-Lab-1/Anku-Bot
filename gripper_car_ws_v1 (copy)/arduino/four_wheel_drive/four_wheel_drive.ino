/*
 * ============================================================================
 * MOBILE BASE CONTROLLER - PRODUCTION VERSION
 * ============================================================================
 * 
 * Four-wheel differential drive robot controller for ROS2 integration
 * 
 * FEATURES:
 * - Closed-loop PID velocity control per wheel
 * - Quadrature encoder feedback with debouncing
 * - Serial communication protocol for ROS2
 * - Differential drive kinematics
 * - Command timeout safety
 * 
 * HARDWARE:
 * - Arduino Mega 2560
 * - 2x L298N H-bridge motor drivers
 * - 4x DC motors with quadrature encoders (360 CPR)
 * 
 * SERIAL PROTOCOL (115200 baud):
 *   ROS -> Arduino:
 *     VEL,<linear_x>,<angular_z>  - Set velocity (m/s, rad/s)
 *     STOP                        - Emergency stop
 *     PID,<kp>,<ki>,<kd>         - Update PID gains
 *     RST                         - Reset encoders
 *     STATUS                      - Query status
 * 
 *   Arduino -> ROS:
 *     ENC,<fl>,<fr>,<bl>,<br>    - Encoder counts (change-based)
 *     STATUS,<message>           - Status messages
 * 
 * AUTHOR: [Your Name]
 * VERSION: 1.0 (Production)
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
#define WHEEL_BASE       0.35     // meters (left-right distance)
#define ENCODER_CPR      360      // Counts per revolution (quadrature decoded)

// Control Loop Timing
#define PID_SAMPLE_TIME  20       // milliseconds (50 Hz)
#define ENCODER_PUB_RATE 50       // Hz
#define CMD_TIMEOUT      2000     // milliseconds

// PID Controller Gains (tunable via serial)
float Kp = 2.0;
float Ki = 0.5;
float Kd = 0.1;

// Encoder Noise Filtering
#define ENCODER_DEBOUNCE_US 200   // microseconds

// Motor Calibration Factors (compensates for manufacturing variance)
// Adjust these to normalize wheel speeds and prevent drift
float motor_cal[4] = {0.902, 0.750, 1.00, 0.80};  // FL, FR, BL, BR

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

// Motor Driver 1 - Front Wheels
#define M_FL_IN1  22    // Front Left direction A
#define M_FL_IN2  23    // Front Left direction B
#define M_FL_EN   12    // Front Left PWM speed control
#define M_FR_IN1  24    // Front Right direction A
#define M_FR_IN2  25    // Front Right direction B
#define M_FR_EN   11    // Front Right PWM speed control

// Motor Driver 2 - Rear Wheels
#define M_BL_IN1  26    // Back Left direction A
#define M_BL_IN2  27    // Back Left direction B
#define M_BL_EN   10    // Back Left PWM speed control
#define M_BR_IN1  28    // Back Right direction A
#define M_BR_IN2  29    // Back Right direction B
#define M_BR_EN   9     // Back Right PWM speed control

// Quadrature Encoders (interrupt-capable pins)
#define ENC_FL_A  18    // Front Left A (INT3)
#define ENC_FL_B  31    // Front Left B
#define ENC_FR_A  19    // Front Right A (INT2)
#define ENC_FR_B  33    // Front Right B
#define ENC_BL_A  20    // Back Left A (INT1)
#define ENC_BL_B  35    // Back Left B
#define ENC_BR_A  21    // Back Right A (INT0)
#define ENC_BR_B  37    // Back Right B

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

// Encoder State (volatile for ISR access)
volatile long enc_counts[4] = {0, 0, 0, 0};           // FL, FR, BL, BR
volatile unsigned long last_isr_time[4] = {0, 0, 0, 0}; // Debounce tracking
long prev_enc_counts[4] = {0, 0, 0, 0};
long last_published_counts[4] = {0, 0, 0, 0};

// Encoder direction mapping (compensates for wiring polarity)
const int encoder_dir[4] = {1, -1, 1, -1};  // FL, FR, BL, BR

// Velocity Control State
float target_vel[4] = {0, 0, 0, 0};         // Target wheel velocities (rad/s)
float current_vel[4] = {0, 0, 0, 0};        // Measured wheel velocities (rad/s)

// PID Controller State
float pid_integral[4] = {0, 0, 0, 0};
float pid_prev_error[4] = {0, 0, 0, 0};

// Timing State
unsigned long last_pid_time = 0;
unsigned long last_enc_pub_time = 0;
unsigned long last_cmd_time = 0;

// Serial Command Buffer
String cmd_buffer = "";

// Motor Control Arrays (for indexed access)
const int motor_in1_pins[4] = {M_FL_IN1, M_FR_IN1, M_BL_IN1, M_BR_IN1};
const int motor_in2_pins[4] = {M_FL_IN2, M_FR_IN2, M_BL_IN2, M_BR_IN2};
const int motor_en_pins[4] = {M_FL_EN, M_FR_EN, M_BL_EN, M_BR_EN};
const int motor_direction[4] = {1, 1, 1, 1};  // Direction polarity

// ============================================================================
// MOTOR CONTROL MODULE
// ============================================================================

/**
 * Initialize motor driver pins and set motors to stopped state
 */
void setupMotors() {
  for (int i = 0; i < 4; i++) {
    pinMode(motor_in1_pins[i], OUTPUT);
    pinMode(motor_in2_pins[i], OUTPUT);
    pinMode(motor_en_pins[i], OUTPUT);
    
    // Initialize to stopped state
    digitalWrite(motor_in1_pins[i], LOW);
    digitalWrite(motor_in2_pins[i], LOW);
    analogWrite(motor_en_pins[i], 0);
  }
}

/**
 * Set motor speed and direction
 * 
 * @param motor_idx Motor index (0-3: FL, FR, BL, BR)
 * @param pwm_like  PWM value: -255 (full reverse) to +255 (full forward)
 */
void setMotorOutput(int motor_idx, int pwm_like) {
  // Apply per-motor calibration
  float scaled_f = pwm_like * motor_cal[motor_idx];
  int scaled_pwm = (int)constrain(scaled_f, -255, 255);
  
  // Apply direction mapping
  int final_pwm = scaled_pwm * motor_direction[motor_idx];
  
  // Extract magnitude and direction
  int pwm_magnitude = abs(final_pwm);
  int direction = (final_pwm > 0) ? 1 : (final_pwm < 0) ? -1 : 0;
  
  // Set direction pins (H-bridge control)
  if (direction > 0) {
    // Forward
    digitalWrite(motor_in1_pins[motor_idx], HIGH);
    digitalWrite(motor_in2_pins[motor_idx], LOW);
  } else if (direction < 0) {
    // Reverse
    digitalWrite(motor_in1_pins[motor_idx], LOW);
    digitalWrite(motor_in2_pins[motor_idx], HIGH);
  } else {
    // Brake (both LOW)
    digitalWrite(motor_in1_pins[motor_idx], LOW);
    digitalWrite(motor_in2_pins[motor_idx], LOW);
  }
  
  // Set speed via PWM
  analogWrite(motor_en_pins[motor_idx], pwm_magnitude);
}

/**
 * Emergency stop - halt all motors and reset control state
 */
void stopAllMotors() {
  for (int i = 0; i < 4; i++) {
    setMotorOutput(i, 0);
    target_vel[i] = 0;
    pid_integral[i] = 0;
    pid_prev_error[i] = 0;
  }
}

// ============================================================================
// ENCODER MODULE
// ============================================================================

/**
 * Initialize encoder pins and attach interrupts
 */
void setupEncoders() {
  // Configure encoder pins as inputs with pull-ups
  pinMode(ENC_FL_A, INPUT_PULLUP);
  pinMode(ENC_FL_B, INPUT_PULLUP);
  pinMode(ENC_FR_A, INPUT_PULLUP);
  pinMode(ENC_FR_B, INPUT_PULLUP);
  pinMode(ENC_BL_A, INPUT_PULLUP);
  pinMode(ENC_BL_B, INPUT_PULLUP);
  pinMode(ENC_BR_A, INPUT_PULLUP);
  pinMode(ENC_BR_B, INPUT_PULLUP);
  
  // Attach interrupts (2x decoding on rising edge of channel A)
  attachInterrupt(digitalPinToInterrupt(ENC_FL_A), isr_enc_fl, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_FR_A), isr_enc_fr, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_BL_A), isr_enc_bl, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_BR_A), isr_enc_br, RISING);
}

/**
 * Interrupt Service Routines (ISR) for quadrature encoders
 * Uses 2x decoding: counts on rising edge of channel A
 * Includes debouncing to filter electrical noise
 */

void isr_enc_fl() {
  unsigned long now = micros();
  if (now - last_isr_time[0] < ENCODER_DEBOUNCE_US) return;
  last_isr_time[0] = now;
  
  enc_counts[0] += digitalRead(ENC_FL_B) ? -1 : 1;
}

void isr_enc_fr() {
  unsigned long now = micros();
  if (now - last_isr_time[1] < ENCODER_DEBOUNCE_US) return;
  last_isr_time[1] = now;
  
  enc_counts[1] += digitalRead(ENC_FR_B) ? 1 : -1;
}

void isr_enc_bl() {
  unsigned long now = micros();
  if (now - last_isr_time[2] < ENCODER_DEBOUNCE_US) return;
  last_isr_time[2] = now;
  
  enc_counts[2] += digitalRead(ENC_BL_B) ? -1 : 1;
}

void isr_enc_br() {
  unsigned long now = micros();
  if (now - last_isr_time[3] < ENCODER_DEBOUNCE_US) return;
  last_isr_time[3] = now;
  
  enc_counts[3] += digitalRead(ENC_BR_B) ? 1 : -1;
}

/**
 * Atomically read encoder counts
 * 
 * @param counts Output array for encoder values (4 elements)
 */
void getEncoderCounts(long* counts) {
  noInterrupts();
  for (int i = 0; i < 4; i++) {
    counts[i] = enc_counts[i];
  }
  interrupts();
}

/**
 * Reset all encoder counts to zero
 */
void resetEncoders() {
  noInterrupts();
  for (int i = 0; i < 4; i++) {
    enc_counts[i] = 0;
    prev_enc_counts[i] = 0;
    last_published_counts[i] = 0;
  }
  interrupts();
}

// ============================================================================
// PID CONTROLLER MODULE
// ============================================================================

/**
 * Compute PID control output
 * 
 * @param motor_idx Motor index (0-3)
 * @param target    Target velocity (rad/s)
 * @param current   Measured velocity (rad/s)
 * @param dt        Time delta (seconds)
 * @return          PWM magnitude (0-255)
 */
float computePID(int motor_idx, float target, float current, float dt) {
  float error = target - current;
  
  // Proportional term
  float p_term = Kp * error;
  
  // Integral term with anti-windup
  pid_integral[motor_idx] += error * dt;
  pid_integral[motor_idx] = constrain(pid_integral[motor_idx], -100, 100);
  float i_term = Ki * pid_integral[motor_idx];
  
  // Derivative term
  float d_term = 0;
  if (dt > 0) {
    d_term = Kd * (error - pid_prev_error[motor_idx]) / dt;
  }
  pid_prev_error[motor_idx] = error;
  
  // Compute output and scale to PWM range
  float output = (p_term + i_term + d_term) * 25.5;  // Scale factor
  
  return constrain(fabs(output), 0, 255);
}

/**
 * Update PID controllers for all wheels
 * Runs at fixed sample rate (PID_SAMPLE_TIME)
 */
void updatePID() {
  unsigned long now = millis();
  float dt = (now - last_pid_time) / 1000.0;
  
  if (dt < (PID_SAMPLE_TIME / 1000.0)) return;
  
  // Get current encoder counts
  long counts[4];
  getEncoderCounts(counts);
  
  // Calculate wheel velocities from encoder deltas
  for (int i = 0; i < 4; i++) {
    long raw_delta = counts[i] - prev_enc_counts[i];
    long delta = raw_delta * encoder_dir[i];  // Normalize direction
    prev_enc_counts[i] = counts[i];
    
    // Convert ticks to rad/s: velocity = (delta_ticks / CPR) * 2π / dt
    current_vel[i] = (delta * 2.0 * PI) / (ENCODER_CPR * dt);
  }
  
  // Update PID for each wheel
  for (int i = 0; i < 4; i++) {
    float vel_error = target_vel[i] - current_vel[i];
    const float VEL_DEADBAND = 0.3;  // rad/s
    
    // If within deadband, consider target reached
    if (fabs(vel_error) < VEL_DEADBAND) {
      setMotorOutput(i, 0);
      pid_integral[i] = 0;
      continue;
    }
    
    // Compute PID output (magnitude only)
    int pwm_mag = (int)computePID(i, fabs(target_vel[i]), fabs(current_vel[i]), dt);
    
    // Apply direction based on target sign
    int pwm = (target_vel[i] >= 0) ? pwm_mag : -pwm_mag;
    
    // Command motor
    setMotorOutput(i, pwm);
  }
  
  last_pid_time = now;
}

// ============================================================================
// DIFFERENTIAL DRIVE KINEMATICS
// ============================================================================

/**
 * Convert twist command to wheel velocities
 * 
 * @param linear_x  Linear velocity (m/s)
 * @param angular_z Angular velocity (rad/s)
 */
void setVelocityCommand(float linear_x, float angular_z) {
  // Differential drive inverse kinematics
  float v_left = linear_x - (angular_z * WHEEL_BASE / 2.0);
  float v_right = linear_x + (angular_z * WHEEL_BASE / 2.0);
  
  // Convert linear to angular velocity: ω = v / r
  float omega_left = v_left / WHEEL_RADIUS;
  float omega_right = v_right / WHEEL_RADIUS;
  
  // Set target velocities (FL & BL = left, FR & BR = right)
  target_vel[0] = omega_left;   // Front Left
  target_vel[1] = omega_right;  // Front Right
  target_vel[2] = omega_left;   // Back Left
  target_vel[3] = omega_right;  // Back Right
  
  // Update watchdog timer
  last_cmd_time = millis();
}

// ============================================================================
// SERIAL COMMUNICATION MODULE
// ============================================================================

/**
 * Process incoming serial command
 * 
 * @param cmd Command string
 */
void processCommand(String cmd) {
  cmd.trim();
  
  if (cmd.startsWith("VEL,")) {
    // Parse: VEL,<linear_x>,<angular_z>
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
    // Parse: PID,<kp>,<ki>,<kd>
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);
    int idx3 = cmd.indexOf(',', idx2 + 1);
    
    if (idx3 > idx2 && idx2 > idx1) {
      Kp = cmd.substring(idx1 + 1, idx2).toFloat();
      Ki = cmd.substring(idx2 + 1, idx3).toFloat();
      Kd = cmd.substring(idx3 + 1).toFloat();
      
      Serial.print("STATUS,PID updated: Kp=");
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
    Serial.print("STATUS,Vel:[");
    Serial.print(target_vel[0], 2);
    Serial.print(",");
    Serial.print(target_vel[1], 2);
    Serial.print(",");
    Serial.print(target_vel[2], 2);
    Serial.print(",");
    Serial.print(target_vel[3], 2);
    Serial.println("]");
  }
}

/**
 * Read and process incoming serial data
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
 * Publish encoder counts to serial (change-based)
 * Only publishes when values have changed to reduce bandwidth
 */
void publishEncoderCounts() {
  unsigned long now = millis();
  unsigned long pub_interval = 1000 / ENCODER_PUB_RATE;
  
  if (now - last_enc_pub_time < pub_interval) return;
  
  long counts[4];
  getEncoderCounts(counts);
  
  // Check if any encoder changed
  bool changed = false;
  for (int i = 0; i < 4; i++) {
    if (counts[i] != last_published_counts[i]) {
      changed = true;
      break;
    }
  }
  
  if (changed) {
    Serial.print("ENC,");
    Serial.print(counts[0]);
    Serial.print(",");
    Serial.print(counts[1]);
    Serial.print(",");
    Serial.print(counts[2]);
    Serial.print(",");
    Serial.println(counts[3]);
    
    // Update last published values
    for (int i = 0; i < 4; i++) {
      last_published_counts[i] = counts[i];
    }
  }
  
  last_enc_pub_time = now;
}

/**
 * Command timeout watchdog
 * Stops motors if no command received within CMD_TIMEOUT
 */
void checkCommandTimeout() {
  unsigned long dt = millis() - last_cmd_time;
  
  if (dt > CMD_TIMEOUT) {
    bool any_moving = false;
    for (int i = 0; i < 4; i++) {
      if (target_vel[i] != 0) {
        any_moving = true;
        break;
      }
    }
    
    if (any_moving) {
      stopAllMotors();
      Serial.println("STATUS,Command timeout - stopped");
    }
  }
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
    ;  // Wait for serial port
  }
  
  // Initialize hardware
  setupMotors();
  setupEncoders();
  
  // Initialize timing
  last_pid_time = millis();
  last_enc_pub_time = millis();
  last_cmd_time = millis();
  
  // Print startup message
  Serial.println("STATUS,Mobile Base Controller Ready [PRODUCTION]");
  Serial.print("STATUS,Wheel: r=");
  Serial.print(WHEEL_RADIUS, 3);
  Serial.print("m, L=");
  Serial.print(WHEEL_BASE, 3);
  Serial.println("m");
  Serial.print("STATUS,Encoder: CPR=");
  Serial.print(ENCODER_CPR);
  Serial.print(", PID: Kp=");
  Serial.print(Kp, 2);
  Serial.print(" Ki=");
  Serial.print(Ki, 2);
  Serial.print(" Kd=");
  Serial.println(Kd, 2);
}

void loop() {
  // Read serial commands
  readSerial();
  
  // Safety: command timeout watchdog
  checkCommandTimeout();
  
  // Control: update PID controllers
  updatePID();
  
  // Communication: publish encoder feedback
  publishEncoderCounts();
}
