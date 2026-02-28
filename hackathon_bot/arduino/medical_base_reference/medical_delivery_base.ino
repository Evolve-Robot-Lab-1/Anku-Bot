/*
 * Medical Delivery Robot - Step 1 base controller
 * Arduino Mega 2560 + two L298N-style motor channels.
 *
 * USB serial protocol at 115200 baud:
 *   VEL,<linear_mps>,<angular_rps>
 *   MOTOR,<left_pwm>,<right_pwm>
 *   STOP
 *   ESTOP
 *   CLEAR_ESTOP
 *   RST
 *   STATUS
 *
 * Replies:
 *   ENC,<left_ticks>,<right_ticks>
 *   STATUS,<message>
 *   HEARTBEAT,<millis>
 *   ERROR,<message>
 */

#include <Arduino.h>

const unsigned long SERIAL_BAUD = 115200;
const float WHEEL_SEPARATION_M = 0.35f;
const float MAX_LINEAR_SPEED_MPS = 0.50f;
const unsigned long COMMAND_TIMEOUT_MS = 500;
const unsigned long ENCODER_REPORT_INTERVAL_MS = 100;
const unsigned long HEARTBEAT_INTERVAL_MS = 1000;

// Provisional pin map adapted from the existing four-wheel Mega project.
const uint8_t LEFT_PWM_PIN = 12;
const uint8_t LEFT_IN1_PIN = 22;
const uint8_t LEFT_IN2_PIN = 23;
const uint8_t RIGHT_PWM_PIN = 11;
const uint8_t RIGHT_IN1_PIN = 24;
const uint8_t RIGHT_IN2_PIN = 25;

const uint8_t LEFT_ENCODER_A_PIN = 18;
const uint8_t LEFT_ENCODER_B_PIN = 31;
const uint8_t RIGHT_ENCODER_A_PIN = 19;
const uint8_t RIGHT_ENCODER_B_PIN = 33;

// Bench default: enable this after the physical active-low E-stop is wired.
const bool USE_PHYSICAL_ESTOP = false;
const uint8_t ESTOP_PIN = 20;

// Change only if a motor is mechanically reversed.
const int8_t LEFT_MOTOR_SIGN = 1;
const int8_t RIGHT_MOTOR_SIGN = 1;
const int8_t LEFT_ENCODER_SIGN = 1;
const int8_t RIGHT_ENCODER_SIGN = 1;

volatile long left_encoder_ticks = 0;
volatile long right_encoder_ticks = 0;
String input_line;
unsigned long last_command_ms = 0;
unsigned long last_encoder_report_ms = 0;
unsigned long last_heartbeat_ms = 0;
bool estop_latched = false;

void leftEncoderISR() {
  const int direction = digitalRead(LEFT_ENCODER_B_PIN) ? -1 : 1;
  left_encoder_ticks += direction * LEFT_ENCODER_SIGN;
}

void rightEncoderISR() {
  const int direction = digitalRead(RIGHT_ENCODER_B_PIN) ? 1 : -1;
  right_encoder_ticks += direction * RIGHT_ENCODER_SIGN;
}

void reportStatus(const String &message) {
  Serial.print("STATUS,");
  Serial.println(message);
}

void setMotor(uint8_t pwm_pin, uint8_t in1_pin, uint8_t in2_pin,
              int pwm_value, int8_t direction_sign) {
  int signed_pwm = constrain(pwm_value, -255, 255) * direction_sign;
  if (signed_pwm > 0) {
    digitalWrite(in1_pin, HIGH);
    digitalWrite(in2_pin, LOW);
  } else if (signed_pwm < 0) {
    digitalWrite(in1_pin, LOW);
    digitalWrite(in2_pin, HIGH);
  } else {
    digitalWrite(in1_pin, LOW);
    digitalWrite(in2_pin, LOW);
  }
  analogWrite(pwm_pin, abs(signed_pwm));
}

void stopMotors() {
  setMotor(LEFT_PWM_PIN, LEFT_IN1_PIN, LEFT_IN2_PIN, 0, LEFT_MOTOR_SIGN);
  setMotor(RIGHT_PWM_PIN, RIGHT_IN1_PIN, RIGHT_IN2_PIN, 0, RIGHT_MOTOR_SIGN);
}

void setMotorPWM(int left_pwm, int right_pwm) {
  if (estop_latched) {
    stopMotors();
    return;
  }
  setMotor(LEFT_PWM_PIN, LEFT_IN1_PIN, LEFT_IN2_PIN, left_pwm, LEFT_MOTOR_SIGN);
  setMotor(RIGHT_PWM_PIN, RIGHT_IN1_PIN, RIGHT_IN2_PIN, right_pwm, RIGHT_MOTOR_SIGN);
}

int velocityToPWM(float velocity_mps) {
  const float limited = constrain(velocity_mps,
                                  -MAX_LINEAR_SPEED_MPS,
                                  MAX_LINEAR_SPEED_MPS);
  return (int)(limited / MAX_LINEAR_SPEED_MPS * 255.0f);
}

bool parseTwoFloats(const String &command, float &first, float &second) {
  const int first_comma = command.indexOf(',');
  if (first_comma < 0) {
    return false;
  }
  const int second_comma = command.indexOf(',', first_comma + 1);
  if (second_comma < 0) {
    return false;
  }
  first = command.substring(first_comma + 1, second_comma).toFloat();
  second = command.substring(second_comma + 1).toFloat();
  return true;
}

void handleCommand(String command) {
  command.trim();
  if (command.length() == 0) {
    return;
  }

  if (command.startsWith("VEL,")) {
    float linear = 0.0f;
    float angular = 0.0f;
    if (!parseTwoFloats(command, linear, angular)) {
      reportStatus("ERROR,VEL format: VEL,<linear>,<angular>");
      return;
    }
    const float left_velocity = linear - (angular * WHEEL_SEPARATION_M / 2.0f);
    const float right_velocity = linear + (angular * WHEEL_SEPARATION_M / 2.0f);
    setMotorPWM(velocityToPWM(left_velocity), velocityToPWM(right_velocity));
    last_command_ms = millis();
    Serial.println("OK,VEL");
    return;
  }

  if (command.startsWith("MOTOR,")) {
    float left = 0.0f;
    float right = 0.0f;
    if (!parseTwoFloats(command, left, right)) {
      reportStatus("ERROR,MOTOR format: MOTOR,<left_pwm>,<right_pwm>");
      return;
    }
    setMotorPWM((int)left, (int)right);
    last_command_ms = millis();
    Serial.println("OK,MOTOR");
    return;
  }

  if (command == "STOP") {
    stopMotors();
    reportStatus("STOPPED");
    return;
  }

  if (command == "ESTOP") {
    estop_latched = true;
    stopMotors();
    reportStatus("ESTOP");
    return;
  }

  if (command == "CLEAR_ESTOP") {
    if (!USE_PHYSICAL_ESTOP || digitalRead(ESTOP_PIN) == HIGH) {
      estop_latched = false;
      reportStatus("ESTOP_CLEARED");
    } else {
      reportStatus("ESTOP_INPUT_ACTIVE");
    }
    return;
  }

  if (command == "RST") {
    noInterrupts();
    left_encoder_ticks = 0;
    right_encoder_ticks = 0;
    interrupts();
    reportStatus("ENCODERS_RESET");
    return;
  }

  if (command == "STATUS") {
    reportStatus(estop_latched ? "ESTOP" : "READY");
    return;
  }

  reportStatus("ERROR,UNKNOWN_COMMAND");
}

void readSerial() {
  while (Serial.available() > 0) {
    const char character = (char)Serial.read();
    if (character == '\n' || character == '\r') {
      handleCommand(input_line);
      input_line = "";
    } else if (input_line.length() < 96) {
      input_line += character;
    } else {
      input_line = "";
      reportStatus("ERROR,COMMAND_TOO_LONG");
    }
  }
}

void reportEncoders() {
  long left;
  long right;
  noInterrupts();
  left = left_encoder_ticks;
  right = right_encoder_ticks;
  interrupts();

  Serial.print("ENC,");
  Serial.print(left);
  Serial.print(",");
  Serial.println(right);
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(LEFT_IN1_PIN, OUTPUT);
  pinMode(LEFT_IN2_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_IN1_PIN, OUTPUT);
  pinMode(RIGHT_IN2_PIN, OUTPUT);
  stopMotors();

  pinMode(LEFT_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(LEFT_ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(RIGHT_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(RIGHT_ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LEFT_ENCODER_A_PIN), leftEncoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_ENCODER_A_PIN), rightEncoderISR, RISING);

  if (USE_PHYSICAL_ESTOP) {
    pinMode(ESTOP_PIN, INPUT_PULLUP);
  }

  last_command_ms = millis();
  reportStatus("READY");
}

void loop() {
  readSerial();

  if (USE_PHYSICAL_ESTOP && digitalRead(ESTOP_PIN) == LOW) {
    estop_latched = true;
    stopMotors();
  }

  if (millis() - last_command_ms > COMMAND_TIMEOUT_MS) {
    stopMotors();
  }

  if (millis() - last_encoder_report_ms >= ENCODER_REPORT_INTERVAL_MS) {
    last_encoder_report_ms = millis();
    reportEncoders();
  }

  if (millis() - last_heartbeat_ms >= HEARTBEAT_INTERVAL_MS) {
    last_heartbeat_ms = millis();
    Serial.print("HEARTBEAT,");
    Serial.println(millis());
  }
}
