/*
 * SIMPLE MOTOR TEST - Arduino Mega 2560
 * No PCA9685/I2C - just motors
 */

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

String cmd_buffer = "";

void setup() {
  Serial.begin(SERIAL_BAUD);

  for (int i = 0; i < 4; i++) {
    pinMode(motor_in1[i], OUTPUT);
    pinMode(motor_in2[i], OUTPUT);
    pinMode(motor_en[i], OUTPUT);
    digitalWrite(motor_in1[i], LOW);
    digitalWrite(motor_in2[i], LOW);
    analogWrite(motor_en[i], 0);
  }

  Serial.println("================================");
  Serial.println("SIMPLE MOTOR TEST - Mega 2560");
  Serial.println("================================");
  Serial.println("Commands:");
  Serial.println("  MOT,<0-3>,<-255 to 255>");
  Serial.println("  STOP");
  Serial.println("  TEST");
  Serial.println("  STATUS");
  Serial.println("================================");
  Serial.println("Motor mapping:");
  Serial.println("  0=FL, 1=FR, 2=BL, 3=BR");
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

void runTest() {
  Serial.println("Starting motor test...");
  for (int i = 0; i < 4; i++) {
    Serial.print("Testing ");
    Serial.print(motor_names[i]);
    Serial.println(" forward...");
    setMotor(i, 150);
    delay(1000);
    setMotor(i, 0);
    delay(500);

    Serial.print("Testing ");
    Serial.print(motor_names[i]);
    Serial.println(" reverse...");
    setMotor(i, -150);
    delay(1000);
    setMotor(i, 0);
    delay(500);
  }
  Serial.println("Test complete!");
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
  else if (cmd == "STOP") {
    stopAll();
  }
  else if (cmd == "TEST") {
    runTest();
  }
  else if (cmd == "STATUS") {
    Serial.println("STATUS,OK");
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
