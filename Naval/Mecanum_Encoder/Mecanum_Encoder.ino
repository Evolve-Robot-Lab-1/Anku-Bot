/*
 * Mecanum Wheel Control with Encoders + PID Speed Equalization + Servo Tilt
 * Per-motor calibration for uneven motor speeds.
 *
 * M1 (FL): EN=2,  IN1=22, IN2=23,  CHA=18, CHB=30
 * M2 (FR): EN=3,  IN1=24, IN2=25,  CHA=19, CHB=31
 * M3 (BL): EN=5,  IN1=28, IN2=29,  CHA=21, CHB=33
 * M4 (BR): EN=4,  IN1=26, IN2=27,  CHA=20, CHB=32
 *
 * Camera Tilt Servos:
 *   Front camera: Pin 9  (TODO: update when wired)
 *   Rear camera:  Pin 10 (TODO: update when wired)
 *
 * Commands:
 *   F = Forward       B = Backward
 *   L = Strafe Left   R = Strafe Right
 *   W = Rotate Left   U = Rotate Right
 *   S = Stop
 *   1 = Slow (180 t/s)  2 = Medium (250 t/s)  3 = Fast (320 t/s)
 *   P = Print encoder ticks
 *   C = Clear encoder ticks
 *   T<angle> = Front camera tilt (0-90 degrees)
 *   G<angle> = Rear camera tilt (0-90 degrees)
 *   CAL = Print calibration factors
 *   CAL,<m1>,<m2>,<m3>,<m4> = Set per-motor PWM scale (0.5-2.0)
 */

#include <Servo.h>

// M1 - Front Left
#define M1_EN   2
#define M1_IN1  22
#define M1_IN2  23
#define M1_CHA  18
#define M1_CHB  30

// M2 - Front Right
#define M2_EN   3
#define M2_IN1  24
#define M2_IN2  25
#define M2_CHA  19
#define M2_CHB  31

// M3 - Back Left (was physically wired as M4)
#define M3_EN   5
#define M3_IN1  28
#define M3_IN2  29
#define M3_CHA  21
#define M3_CHB  33

// M4 - Back Right (was physically wired as M3)
#define M4_EN   4
#define M4_IN1  26
#define M4_IN2  27
#define M4_CHA  20
#define M4_CHB  32

// Camera tilt servo pins (TODO: update when physically wired)
#define SERVO_FRONT_PIN  9
#define SERVO_REAR_PIN   10

Servo servoFront;
Servo servoRear;

// Encoder tick counts
volatile long encM1 = 0;
volatile long encM2 = 0;
volatile long encM3 = 0;
volatile long encM4 = 0;

int basePwm = 180;        // base PWM before per-motor scaling
int userTarget = 180;      // desired speed in ticks/s (set by 1/2/3)
bool moving = false;

// Per-motor calibration: PWM multiplier to compensate for motor differences
// M1(FL) is ~3x slower than others at same PWM, so it gets higher scale
float motorCal[4] = {1.42, 1.0, 1.42, 1.3};  // M1=255, M2=180, M3=255, M4=234
// Per-motor max speed (t/s) measured during calibration
float motorMaxSpd[4] = {0, 0, 0, 0};

// Speed measurement - use 200ms window for stability
long prevEnc[4] = {0, 0, 0, 0};
float speed[4] = {0, 0, 0, 0};
unsigned long lastPidTime = 0;
#define PID_INTERVAL 200  // measure + PID every 200ms
unsigned long lastPrintTime = 0;
#define PRINT_INTERVAL 600  // print every 600ms

// PID
float Kp = 0.4;
float Ki = 0.15;
float Kd = 0.05;
float errSum[4] = {0, 0, 0, 0};
float lastErr[4] = {0, 0, 0, 0};
int pwmOut[4] = {0, 0, 0, 0};

float targetSpeed = 0;
int motorDir[4] = {0, 0, 0, 0};

// Calibration: wait 1s for ramp-up, then measure for 1s
bool calibrating = false;
unsigned long calibStartTime = 0;
bool calibMeasuring = false;
long calibEncStart[4] = {0, 0, 0, 0};
unsigned long calibMeasureStart = 0;

// Serial input buffer for multi-char commands (T90, G45, etc.)
char serialBuf[16];
int serialBufIdx = 0;

// --- Encoder ISRs ---
void isrM1() { if (digitalRead(M1_CHB)) encM1++; else encM1--; }
void isrM2() { if (digitalRead(M2_CHB)) encM2--; else encM2++; }
void isrM3() { if (digitalRead(M3_CHB)) encM3++; else encM3--; }
void isrM4() { if (digitalRead(M4_CHB)) encM4--; else encM4++; }

// --- Motor drive ---
void driveMotor(int mIdx, int dir, int pwm) {
  switch (mIdx) {
    case 0: // M1
      if (dir > 0) { digitalWrite(M1_IN1, HIGH); digitalWrite(M1_IN2, LOW); }
      else if (dir < 0) { digitalWrite(M1_IN1, LOW); digitalWrite(M1_IN2, HIGH); }
      else { digitalWrite(M1_IN1, LOW); digitalWrite(M1_IN2, LOW); }
      analogWrite(M1_EN, pwm);
      break;
    case 1: // M2
      if (dir > 0) { digitalWrite(M2_IN1, HIGH); digitalWrite(M2_IN2, LOW); }
      else if (dir < 0) { digitalWrite(M2_IN1, LOW); digitalWrite(M2_IN2, HIGH); }
      else { digitalWrite(M2_IN1, LOW); digitalWrite(M2_IN2, LOW); }
      analogWrite(M2_EN, pwm);
      break;
    case 2: // M3
      if (dir > 0) { digitalWrite(M3_IN1, HIGH); digitalWrite(M3_IN2, LOW); }
      else if (dir < 0) { digitalWrite(M3_IN1, LOW); digitalWrite(M3_IN2, HIGH); }
      else { digitalWrite(M3_IN1, LOW); digitalWrite(M3_IN2, LOW); }
      analogWrite(M3_EN, pwm);
      break;
    case 3: // M4
      if (dir > 0) { digitalWrite(M4_IN1, HIGH); digitalWrite(M4_IN2, LOW); }
      else if (dir < 0) { digitalWrite(M4_IN1, LOW); digitalWrite(M4_IN2, HIGH); }
      else { digitalWrite(M4_IN1, LOW); digitalWrite(M4_IN2, LOW); }
      analogWrite(M4_EN, pwm);
      break;
  }
}

void stopAll() {
  for (int i = 0; i < 4; i++) driveMotor(i, 0, 0);
}

void readEncoders(long* enc) {
  noInterrupts();
  enc[0] = encM1; enc[1] = encM2; enc[2] = encM3; enc[3] = encM4;
  interrupts();
}

void calcSpeeds() {
  long enc[4];
  readEncoders(enc);
  float dt = PID_INTERVAL / 1000.0;
  for (int i = 0; i < 4; i++) {
    speed[i] = abs(enc[i] - prevEnc[i]) / dt;
    prevEnc[i] = enc[i];
  }
}

int motorStartPwm(int idx) {
  return constrain((int)(basePwm * motorCal[idx]), 100, 255);
}

void pidReset() {
  for (int i = 0; i < 4; i++) {
    errSum[i] = 0;
    lastErr[i] = 0;
    pwmOut[i] = motorStartPwm(i);
  }
}

void pidUpdate() {
  calcSpeeds();
  if (!moving || targetSpeed == 0) return;

  for (int i = 0; i < 4; i++) {
    if (motorDir[i] != 0) {
      float err = targetSpeed - speed[i];
      errSum[i] += err;
      errSum[i] = constrain(errSum[i], -300, 300);
      float dErr = err - lastErr[i];
      lastErr[i] = err;
      float out = Kp * err + Ki * errSum[i] + Kd * dErr;
      pwmOut[i] = constrain(pwmOut[i] + (int)out, 100, 255);
      driveMotor(i, motorDir[i], pwmOut[i]);
    } else {
      pwmOut[i] = 0;
      driveMotor(i, 0, 0);
    }
  }
}

void calibUpdate() {
  unsigned long now = millis();

  // Phase 1: wait 1 second for motors to ramp up
  if (!calibMeasuring) {
    if (now - calibStartTime >= 1000) {
      // Start measuring
      calibMeasuring = true;
      calibMeasureStart = now;
      readEncoders(calibEncStart);
    }
    return;
  }

  // Phase 2: measure for 1 second
  if (now - calibMeasureStart >= 1000) {
    long encNow[4];
    readEncoders(encNow);
    float dt = (now - calibMeasureStart) / 1000.0;

    // Measure each motor's speed
    float calibSpd[4] = {0, 0, 0, 0};
    int activeCount = 0;
    Serial.print(">> Calib speeds: ");
    for (int i = 0; i < 4; i++) {
      if (motorDir[i] != 0) {
        calibSpd[i] = abs(encNow[i] - calibEncStart[i]) / dt;
        motorMaxSpd[i] = calibSpd[i];  // store per-motor max
        Serial.print("M"); Serial.print(i + 1); Serial.print("="); Serial.print(calibSpd[i], 0); Serial.print("  ");
        activeCount++;
      }
    }
    Serial.println();

    // Find 2nd-slowest speed (ignore dead motors <10 t/s)
    // Sort active speeds to find a good target
    float sorted[4];
    int sortCount = 0;
    for (int i = 0; i < 4; i++) {
      if (motorDir[i] != 0 && calibSpd[i] > 10) {
        sorted[sortCount++] = calibSpd[i];
      }
    }
    // Simple bubble sort
    for (int i = 0; i < sortCount - 1; i++)
      for (int j = i + 1; j < sortCount; j++)
        if (sorted[j] < sorted[i]) { float tmp = sorted[i]; sorted[i] = sorted[j]; sorted[j] = tmp; }

    if (sortCount >= 2) {
      // Use 2nd-slowest as target (index 1) - gives weak motor a chance
      float baseTarget = sorted[1];
      if (userTarget > 0 && userTarget < baseTarget) {
        targetSpeed = userTarget;
      } else {
        targetSpeed = baseTarget;
      }
      Serial.print(">> Target (2nd-slowest): "); Serial.println(targetSpeed, 0);
    } else if (sortCount == 1) {
      targetSpeed = sorted[0];
      Serial.print(">> Target (only motor): "); Serial.println(targetSpeed, 0);
    }

    // Initialize PID with per-motor calibrated PWM
    for (int i = 0; i < 4; i++) {
      pwmOut[i] = motorStartPwm(i);
      prevEnc[i] = encNow[i];
    }
    calibrating = false;
  }
}

void startMove(int d1, int d2, int d3, int d4) {
  motorDir[0] = d1; motorDir[1] = d2; motorDir[2] = d3; motorDir[3] = d4;
  pidReset();
  targetSpeed = 0;
  calibrating = true;
  calibMeasuring = false;
  calibStartTime = millis();

  for (int i = 0; i < 4; i++)
    driveMotor(i, motorDir[i], motorStartPwm(i));
  moving = true;
}

void printSpeed() {
  Serial.print("Spd ");
  for (int i = 0; i < 4; i++) {
    Serial.print("M"); Serial.print(i + 1); Serial.print("="); Serial.print(speed[i], 0); Serial.print(" ");
  }
  Serial.print(" | PWM ");
  for (int i = 0; i < 4; i++) {
    Serial.print("M"); Serial.print(i + 1); Serial.print("="); Serial.print(pwmOut[i]); Serial.print(" ");
  }
  Serial.print(" T="); Serial.println(targetSpeed, 0);
}

void printEncoders() {
  long enc[4];
  readEncoders(enc);
  Serial.print("Ticks -> M1="); Serial.print(enc[0]);
  Serial.print("  M2="); Serial.print(enc[1]);
  Serial.print("  M3="); Serial.print(enc[2]);
  Serial.print("  M4="); Serial.println(enc[3]);
}

void clearEncoders() {
  noInterrupts();
  encM1 = 0; encM2 = 0; encM3 = 0; encM4 = 0;
  interrupts();
  Serial.println(">> Encoders cleared");
}

void setServoFront(int angle) {
  angle = constrain(angle, 0, 90);
  servoFront.write(angle);
  Serial.print(">> Front tilt: "); Serial.println(angle);
}

void setServoRear(int angle) {
  angle = constrain(angle, 0, 90);
  servoRear.write(angle);
  Serial.print(">> Rear tilt: "); Serial.println(angle);
}

void processSerialLine(char* buf, int len) {
  if (len == 0) return;

  char cmd = buf[0];

  // Multi-char commands: T<angle>, G<angle>
  if ((cmd == 'T' || cmd == 't') && len > 1) {
    int angle = atoi(&buf[1]);
    setServoFront(angle);
    return;
  }
  if ((cmd == 'G' || cmd == 'g') && len > 1) {
    int angle = atoi(&buf[1]);
    setServoRear(angle);
    return;
  }

  // CAL command: print or set per-motor calibration
  if (len >= 3 && (buf[0] == 'C' || buf[0] == 'c') && (buf[1] == 'A' || buf[1] == 'a') && (buf[2] == 'L' || buf[2] == 'l')) {
    if (len > 4 && buf[3] == ',') {
      // Parse CAL,m1,m2,m3,m4
      float vals[4];
      int vi = 0;
      char* tok = &buf[4];
      for (int i = 4; i <= len && vi < 4; i++) {
        if (i == len || buf[i] == ',') {
          buf[i] = '\0';
          vals[vi++] = atof(tok);
          tok = &buf[i + 1];
        }
      }
      if (vi == 4) {
        for (int i = 0; i < 4; i++) motorCal[i] = constrain(vals[i], 0.5, 2.0);
        Serial.print(">> Cal set: ");
      }
    } else {
      Serial.print(">> Cal: ");
    }
    for (int i = 0; i < 4; i++) {
      Serial.print("M"); Serial.print(i + 1); Serial.print("="); Serial.print(motorCal[i], 2); Serial.print(" ");
    }
    Serial.print(" startPWM: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(motorStartPwm(i)); Serial.print(" ");
    }
    Serial.println();
    return;
  }

  // Single motor test: M<1-4><F/B>[PWM] e.g. M2F or M2F80
  if ((cmd == 'M' || cmd == 'm') && len >= 3) {
    int mIdx = buf[1] - '1';  // 0-3
    char mDir = buf[2];
    if (mIdx >= 0 && mIdx < 4 && (mDir == 'F' || mDir == 'f' || mDir == 'B' || mDir == 'b')) {
      stopAll();
      moving = false;
      calibrating = false;
      int dir = (mDir == 'F' || mDir == 'f') ? 1 : -1;
      int pwm = (len > 3) ? atoi(&buf[3]) : 150;
      pwm = constrain(pwm, 30, 255);
      driveMotor(mIdx, dir, pwm);
      Serial.print(">> Test M"); Serial.print(mIdx + 1);
      Serial.print(dir > 0 ? " FWD" : " REV");
      Serial.print(" PWM="); Serial.println(pwm);
      return;
    }
  }

  // Single-char commands
  switch (cmd) {
    case 'F': case 'f':
      startMove(1, 1, 1, 1);
      Serial.println(">> Forward");
      break;
    case 'B': case 'b':
      startMove(-1, -1, -1, -1);
      Serial.println(">> Backward");
      break;
    case 'L': case 'l':
      startMove(-1, 1, 1, -1);
      Serial.println(">> Strafe Left");
      break;
    case 'R': case 'r':
      startMove(1, -1, -1, 1);
      Serial.println(">> Strafe Right");
      break;
    case 'W': case 'w':
      startMove(-1, 1, -1, 1);
      Serial.println(">> Rotate Left");
      break;
    case 'U': case 'u':
      startMove(1, -1, 1, -1);
      Serial.println(">> Rotate Right");
      break;
    case 'S': case 's':
      stopAll();
      moving = false;
      calibrating = false;
      pidReset();
      Serial.println(">> Stop");
      break;
    case '1':
      userTarget = 180;
      if (moving && !calibrating) { targetSpeed = userTarget; pidReset(); }
      Serial.println(">> Speed: 180 t/s (slow)");
      break;
    case '2':
      userTarget = 250;
      if (moving && !calibrating) { targetSpeed = userTarget; pidReset(); }
      Serial.println(">> Speed: 250 t/s (medium)");
      break;
    case '3':
      userTarget = 320;
      if (moving && !calibrating) { targetSpeed = userTarget; pidReset(); }
      Serial.println(">> Speed: 320 t/s (fast)");
      break;
    case 'P': case 'p':
      printEncoders();
      break;
    case 'C': case 'c':
      clearEncoders();
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(M1_EN, OUTPUT); pinMode(M1_IN1, OUTPUT); pinMode(M1_IN2, OUTPUT);
  pinMode(M2_EN, OUTPUT); pinMode(M2_IN1, OUTPUT); pinMode(M2_IN2, OUTPUT);
  pinMode(M3_EN, OUTPUT); pinMode(M3_IN1, OUTPUT); pinMode(M3_IN2, OUTPUT);
  pinMode(M4_EN, OUTPUT); pinMode(M4_IN1, OUTPUT); pinMode(M4_IN2, OUTPUT);

  pinMode(M1_CHA, INPUT_PULLUP); pinMode(M1_CHB, INPUT_PULLUP);
  pinMode(M2_CHA, INPUT_PULLUP); pinMode(M2_CHB, INPUT_PULLUP);
  pinMode(M3_CHA, INPUT_PULLUP); pinMode(M3_CHB, INPUT_PULLUP);
  pinMode(M4_CHA, INPUT_PULLUP); pinMode(M4_CHB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(M1_CHA), isrM1, RISING);
  attachInterrupt(digitalPinToInterrupt(M2_CHA), isrM2, RISING);
  attachInterrupt(digitalPinToInterrupt(M3_CHA), isrM3, RISING);
  attachInterrupt(digitalPinToInterrupt(M4_CHA), isrM4, RISING);

  // Attach camera tilt servos and set to 0 (level)
  servoFront.attach(SERVO_FRONT_PIN);
  servoRear.attach(SERVO_REAR_PIN);
  servoFront.write(0);
  servoRear.write(0);

  stopAll();
  Serial.println("=== Mecanum PID + Servo Tilt ===");
  Serial.println("F/B/L/R/W/U/S  1=180 2=250 3=320  P=Ticks C=Clear");
  Serial.println("T<0-90>=Front tilt  G<0-90>=Rear tilt  CAL=Show/set cal");
  Serial.print(">> Motor cal: ");
  for (int i = 0; i < 4; i++) {
    Serial.print("M"); Serial.print(i + 1); Serial.print("="); Serial.print(motorCal[i], 2); Serial.print("("); Serial.print(motorStartPwm(i)); Serial.print(") ");
  }
  Serial.println();
}

void loop() {
  // Read serial into buffer, process on newline
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBufIdx > 0) {
        serialBuf[serialBufIdx] = '\0';
        processSerialLine(serialBuf, serialBufIdx);
        serialBufIdx = 0;
      }
    } else if (serialBufIdx < 15) {
      serialBuf[serialBufIdx++] = c;
    }
  }

  if (moving && calibrating) {
    calibUpdate();
  }

  if (moving && !calibrating && millis() - lastPidTime >= PID_INTERVAL) {
    lastPidTime = millis();
    pidUpdate();
  }

  if (moving && millis() - lastPrintTime >= PRINT_INTERVAL) {
    lastPrintTime = millis();
    printSpeed();
  }
}
