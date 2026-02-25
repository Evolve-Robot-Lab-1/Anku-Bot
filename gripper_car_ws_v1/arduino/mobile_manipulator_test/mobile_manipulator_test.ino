/*
 * MOBILE MANIPULATOR TEST FIRMWARE - Arduino Uno Compatible
 * ===========================================================
 * Minimal firmware for testing ROS2 communication without hardware.
 * Works on Arduino Uno - no PCA9685 or encoders required.
 *
 * Features:
 * - Serial command parsing (same protocol as full firmware)
 * - Simulated encoder feedback in TEST mode
 * - Heartbeat messages
 * - Arm position tracking (no actual servo control)
 *
 * Serial Protocol (115200 baud):
 *   ROS -> Arduino:
 *     ARM,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>  - Set arm angles (0-180)
 *     VEL,<linear_x>,<angular_z>         - Velocity command
 *     STOP                               - Emergency stop
 *     TEST,ON/OFF                        - Enable/disable test mode
 *     STATUS                             - Query status
 *     RST                                - Reset encoders
 *
 *   Arduino -> ROS:
 *     ARM_POS,<a0>,...,<a5>             - Arm servo positions
 *     ENC,<fl>,<fr>,<bl>,<br>           - Encoder ticks
 *     STATUS,<message>                  - Status messages
 *     HEARTBEAT,<millis>                - Heartbeat every 1s
 *     OK,<command>                      - Command acknowledgment
 */

// Configuration
#define SERIAL_BAUD 115200
#define WHEEL_RADIUS 0.075
#define WHEEL_BASE 0.35
#define ENCODER_CPR 360
#define HEARTBEAT_INTERVAL 1000
#define ENCODER_PUB_RATE 50

// Arm state (simulated)
float armCurrentAngle[6] = {90, 90, 90, 90, 90, 90};
float armTargetAngle[6] = {90, 90, 90, 90, 90, 90};

// Base state
float target_linear = 0.0;
float target_angular = 0.0;

// Simulated encoder counts
long sim_enc_counts[4] = {0, 0, 0, 0};
float sim_enc_accumulator[4] = {0.0, 0.0, 0.0, 0.0};

// Timing
unsigned long last_heartbeat = 0;
unsigned long last_enc_pub = 0;
unsigned long last_sim_update = 0;

// Test mode
bool test_mode = false;

// Command buffer
String cmd_buffer = "";

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) { ; }

  delay(100);

  Serial.println("STATUS,Mobile Manipulator Test Controller Ready (Uno Compatible)");
  Serial.println("STATUS,Commands: ARM, VEL, STOP, TEST, STATUS, RST");

  last_heartbeat = millis();
  last_enc_pub = millis();
  last_sim_update = millis();
}

void loop() {
  // Read serial commands
  readSerial();

  // Update simulated encoders (in test mode)
  if (test_mode) {
    updateSimulatedEncoders();
  }

  // Update arm positions (smooth transition)
  updateArmPositions();

  // Publish encoder counts
  publishEncoderCounts();

  // Send heartbeat
  sendHeartbeat();
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

void processCommand(String cmd) {
  cmd.trim();

  // ARM command
  if (cmd.startsWith("ARM,")) {
    String t = cmd.substring(4);
    float vals[6];
    int index = 0, last = 0;

    for (unsigned int i = 0; i <= t.length() && index < 6; i++) {
      if (i == t.length() || t.charAt(i) == ',') {
        vals[index++] = t.substring(last, i).toFloat();
        last = i + 1;
      }
    }

    if (index == 6) {
      for (int i = 0; i < 6; i++) {
        armTargetAngle[i] = constrain(vals[i], 0, 180);
      }
      Serial.println("OK,ARM");
    } else {
      Serial.println("ERROR,ARM format: ARM,<a0>,<a1>,<a2>,<a3>,<a4>,<a5>");
    }
  }
  // VEL command
  else if (cmd.startsWith("VEL,")) {
    int idx1 = cmd.indexOf(',');
    int idx2 = cmd.indexOf(',', idx1 + 1);

    if (idx2 > idx1) {
      target_linear = cmd.substring(idx1 + 1, idx2).toFloat();
      target_angular = cmd.substring(idx2 + 1).toFloat();
      Serial.println("OK,VEL");
    }
  }
  // STOP command
  else if (cmd == "STOP") {
    target_linear = 0;
    target_angular = 0;
    for (int i = 0; i < 6; i++) {
      armTargetAngle[i] = armCurrentAngle[i];
    }
    Serial.println("STATUS,Emergency stop - all systems stopped");
  }
  // TEST mode
  else if (cmd == "TEST,ON") {
    test_mode = true;
    resetEncoders();
    Serial.println("STATUS,Test mode ENABLED - simulating encoder feedback");
  }
  else if (cmd == "TEST,OFF") {
    test_mode = false;
    target_linear = 0;
    target_angular = 0;
    Serial.println("STATUS,Test mode DISABLED");
  }
  // STATUS query
  else if (cmd == "STATUS") {
    Serial.print("STATUS,Mode:");
    Serial.print(test_mode ? "TEST" : "NORMAL");
    Serial.print(",Vel:[");
    Serial.print(target_linear, 3);
    Serial.print(",");
    Serial.print(target_angular, 3);
    Serial.print("],ArmTarget:[");
    for (int i = 0; i < 6; i++) {
      Serial.print(armTargetAngle[i], 1);
      if (i < 5) Serial.print(",");
    }
    Serial.println("]");
  }
  // RST - reset encoders
  else if (cmd == "RST") {
    resetEncoders();
    Serial.println("STATUS,Encoders reset");
  }
  else {
    Serial.print("ERROR,Unknown command: ");
    Serial.println(cmd);
  }
}

void resetEncoders() {
  for (int i = 0; i < 4; i++) {
    sim_enc_counts[i] = 0;
    sim_enc_accumulator[i] = 0.0;
  }
}

void updateSimulatedEncoders() {
  unsigned long now = millis();
  float dt = (now - last_sim_update) / 1000.0;

  if (dt <= 0 || dt > 0.1) {
    last_sim_update = now;
    return;
  }

  // Convert velocity to wheel velocities (differential drive)
  float v_left = target_linear - (target_angular * WHEEL_BASE / 2.0);
  float v_right = target_linear + (target_angular * WHEEL_BASE / 2.0);

  // Convert to wheel angular velocities
  float omega_left = v_left / WHEEL_RADIUS;
  float omega_right = v_right / WHEEL_RADIUS;

  // Calculate encoder ticks
  float left_ticks = (omega_left * dt * ENCODER_CPR) / (2.0 * PI);
  float right_ticks = (omega_right * dt * ENCODER_CPR) / (2.0 * PI);

  // Accumulate for all 4 wheels (left side / right side)
  sim_enc_accumulator[0] += left_ticks;   // Front Left
  sim_enc_accumulator[1] += right_ticks;  // Front Right
  sim_enc_accumulator[2] += left_ticks;   // Back Left
  sim_enc_accumulator[3] += right_ticks;  // Back Right

  // Convert accumulated fractional ticks to whole ticks
  for (int i = 0; i < 4; i++) {
    long whole_ticks = (long)sim_enc_accumulator[i];
    if (whole_ticks != 0) {
      sim_enc_counts[i] += whole_ticks;
      sim_enc_accumulator[i] -= whole_ticks;
    }
  }

  last_sim_update = now;
}

void updateArmPositions() {
  bool changed = false;

  for (int i = 0; i < 6; i++) {
    if (armCurrentAngle[i] < armTargetAngle[i]) {
      armCurrentAngle[i] += 1.0;
      if (armCurrentAngle[i] > armTargetAngle[i]) {
        armCurrentAngle[i] = armTargetAngle[i];
      }
      changed = true;
    }
    else if (armCurrentAngle[i] > armTargetAngle[i]) {
      armCurrentAngle[i] -= 1.0;
      if (armCurrentAngle[i] < armTargetAngle[i]) {
        armCurrentAngle[i] = armTargetAngle[i];
      }
      changed = true;
    }
  }

  // Send arm positions when changed
  static unsigned long lastArmPub = 0;
  if (changed && millis() - lastArmPub > 30) {
    Serial.print("ARM_POS,");
    for (int i = 0; i < 6; i++) {
      Serial.print(armCurrentAngle[i], 1);
      if (i < 5) Serial.print(",");
    }
    Serial.println();
    lastArmPub = millis();
  }
}

void publishEncoderCounts() {
  unsigned long now = millis();
  unsigned long interval = 1000 / ENCODER_PUB_RATE;

  if (now - last_enc_pub >= interval) {
    // Only publish if in test mode and there's movement
    if (test_mode && (target_linear != 0 || target_angular != 0)) {
      Serial.print("ENC,");
      Serial.print(sim_enc_counts[0]);
      Serial.print(",");
      Serial.print(sim_enc_counts[1]);
      Serial.print(",");
      Serial.print(sim_enc_counts[2]);
      Serial.print(",");
      Serial.println(sim_enc_counts[3]);
    }
    last_enc_pub = now;
  }
}

void sendHeartbeat() {
  unsigned long now = millis();
  if (now - last_heartbeat >= HEARTBEAT_INTERVAL) {
    Serial.print("HEARTBEAT,");
    Serial.println(now);
    last_heartbeat = now;
  }
}
