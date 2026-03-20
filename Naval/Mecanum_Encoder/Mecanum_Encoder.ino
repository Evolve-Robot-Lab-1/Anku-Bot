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
 *   Front camera: Pin 9
 *   Rear camera:  Pin 10
 *
 * FlySky FS-R6B receiver (pin-change interrupt driven):
 *   CH1 (rotate):   D12  (PB6, PCINT6)
 *   CH2 (throttle):  D11  (PB5, PCINT5)
 *   CH3 (speed):    A8   (PK0, PCINT16)
 *   CH4 (strafe):   A9   (PK1, PCINT17)
 *   CH5 (arm/disarm): A10  (PK2, PCINT18)
 *
 * Commands:
 *   F = Forward       B = Backward
 *   L = Strafe Left   R = Strafe Right
 *   W = Rotate Left   U = Rotate Right
 *   S = Stop (also emergency stop — overrides RF)
 *   1 = Slow (180 t/s)  2 = Medium (250 t/s)  3 = Fast (320 t/s)
 *   P = Print encoder ticks
 *   C = Clear encoder ticks
 *   T<angle> = Front camera tilt (0-90 degrees)
 *   G<angle> = Rear camera tilt (0-90 degrees)
 *   CAL = Print calibration factors
 *   CAL,<m1>,<m2>,<m3>,<m4> = Set per-motor PWM scale (0.5-2.0)
 *   FlySky receiver input is additive and does not replace USB serial control.
 */

#include <Servo.h>
#include <ctype.h>

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

// Camera tilt servo pins
#define SERVO_FRONT_PIN  9
#define SERVO_REAR_PIN   10

// FlySky FS-R6B receiver pins (all PCINT-capable)
// CH2 moved off D13 (built-in LED attenuates signal)
// CH3-CH5 moved off D46-D48 (no PCINT support) to A8-A10
#define RF_CH1_PIN  12   // PB6, PCINT6
#define RF_CH2_PIN  11   // PB5, PCINT5
#define RF_CH3_PIN  A8   // PK0, PCINT16
#define RF_CH4_PIN  A9   // PK1, PCINT17
#define RF_CH5_PIN  A10  // PK2, PCINT18

#define RF_CHANNELS 5

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

enum ControlSource {
  SRC_NONE = 0,
  SRC_ROS,
  SRC_RF
};

// RF pulse width limits and thresholds
const uint16_t RF_PULSE_MIN = 900;
const uint16_t RF_PULSE_MAX = 2100;
const uint16_t RF_ARM_HIGH_US = 1700;
const uint16_t RF_ARM_LOW_US = 1300;
const unsigned long RF_SIGNAL_TIMEOUT_MS = 250;
const unsigned long RF_ARM_HOLD_MS = 500;
const unsigned long ROS_ACTIVITY_TIMEOUT_MS = 800;
const int RF_CENTER_DEADBAND_US = 80;
const int RF_GLITCH_THRESHOLD_US = 200;

// --- RF interrupt-driven pulse capture (written by ISRs) ---
volatile uint16_t rfPulseRaw[RF_CHANNELS] = {1500, 1500, 1500, 1500, 1000};
volatile unsigned long rfRiseTime[RF_CHANNELS] = {0, 0, 0, 0, 0};
volatile bool rfNewData[RF_CHANNELS] = {false, false, false, false, false};
volatile uint8_t lastPINB = 0;
volatile uint8_t lastPINK = 0;

// Filtered RF state (read in main loop)
uint16_t rfPulseUs[RF_CHANNELS] = {1500, 1500, 1500, 1500, 1000};
uint16_t rfPulsePrev[RF_CHANNELS] = {1500, 1500, 1500, 1500, 1000};
unsigned long rfChanLastMs[RF_CHANNELS] = {0, 0, 0, 0, 0};

unsigned long rfLastFramePrintMs = 0;
unsigned long lastRosCmdMs = 0;
ControlSource activeSource = SRC_NONE;
char activeMotionCmd = 'S';
char lastReportedMotionCmd = '\0';
bool rfArmed = false;
bool rfLastArmed = false;
bool rfSignalPresent = false;
bool rfLastSignalPresent = false;
bool rfSeenDisarm = false;
unsigned long rfArmRequestStartMs = 0;

// --- PCINT0 ISR: Port B — CH1 (PB6/D12), CH2 (PB5/D11) ---
ISR(PCINT0_vect) {
  unsigned long now = micros();
  uint8_t portB = PINB;
  uint8_t changed = portB ^ lastPINB;
  lastPINB = portB;

  // CH1 = PB6 (D12)
  if (changed & (1 << PB6)) {
    if (portB & (1 << PB6)) {
      rfRiseTime[0] = now;
    } else {
      uint16_t w = (uint16_t)(now - rfRiseTime[0]);
      if (w >= RF_PULSE_MIN && w <= RF_PULSE_MAX) {
        rfPulseRaw[0] = w;
        rfNewData[0] = true;
      }
    }
  }

  // CH2 = PB5 (D11)
  if (changed & (1 << PB5)) {
    if (portB & (1 << PB5)) {
      rfRiseTime[1] = now;
    } else {
      uint16_t w = (uint16_t)(now - rfRiseTime[1]);
      if (w >= RF_PULSE_MIN && w <= RF_PULSE_MAX) {
        rfPulseRaw[1] = w;
        rfNewData[1] = true;
      }
    }
  }
}

// --- PCINT2 ISR: Port K — CH3 (PK0/A8), CH4 (PK1/A9), CH5 (PK2/A10) ---
ISR(PCINT2_vect) {
  unsigned long now = micros();
  uint8_t portK = PINK;
  uint8_t changed = portK ^ lastPINK;
  lastPINK = portK;

  // CH3 = PK0 (A8)
  if (changed & (1 << PK0)) {
    if (portK & (1 << PK0)) {
      rfRiseTime[2] = now;
    } else {
      uint16_t w = (uint16_t)(now - rfRiseTime[2]);
      if (w >= RF_PULSE_MIN && w <= RF_PULSE_MAX) {
        rfPulseRaw[2] = w;
        rfNewData[2] = true;
      }
    }
  }

  // CH4 = PK1 (A9)
  if (changed & (1 << PK1)) {
    if (portK & (1 << PK1)) {
      rfRiseTime[3] = now;
    } else {
      uint16_t w = (uint16_t)(now - rfRiseTime[3]);
      if (w >= RF_PULSE_MIN && w <= RF_PULSE_MAX) {
        rfPulseRaw[3] = w;
        rfNewData[3] = true;
      }
    }
  }

  // CH5 = PK2 (A10)
  if (changed & (1 << PK2)) {
    if (portK & (1 << PK2)) {
      rfRiseTime[4] = now;
    } else {
      uint16_t w = (uint16_t)(now - rfRiseTime[4]);
      if (w >= RF_PULSE_MIN && w <= RF_PULSE_MAX) {
        rfPulseRaw[4] = w;
        rfNewData[4] = true;
      }
    }
  }
}

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

void reportSource(ControlSource src) {
  if (activeSource == src) return;
  activeSource = src;
  Serial.print(">> Control source: ");
  switch (src) {
    case SRC_RF: Serial.println("RF"); break;
    case SRC_ROS: Serial.println("ROS"); break;
    default: Serial.println("NONE"); break;
  }
}

void applyStop(bool announce = false) {
  stopAll();
  moving = false;
  calibrating = false;
  calibMeasuring = false;
  targetSpeed = 0;
  activeMotionCmd = 'S';
  lastReportedMotionCmd = 'S';
  pidReset();
  if (announce) Serial.println(">> Stop");
}

void setUserSpeedTarget(int newTarget, bool announce = false) {
  userTarget = newTarget;
  if (moving && !calibrating) {
    targetSpeed = userTarget;
    pidReset();
  }
  if (announce) {
    Serial.print(">> Speed: ");
    Serial.print(newTarget);
    if (newTarget == 180) Serial.println(" t/s (slow)");
    else if (newTarget == 250) Serial.println(" t/s (medium)");
    else if (newTarget == 320) Serial.println(" t/s (fast)");
    else Serial.println(" t/s");
  }
}

void applyMotionCommand(char cmd, bool announce = false) {
  char normalized = toupper(cmd);

  if (normalized == 'S') {
    if (activeMotionCmd != 'S' || moving || calibrating) {
      applyStop(announce);
    } else if (announce) {
      Serial.println(">> Stop");
    }
    return;
  }

  int desired[4] = {0, 0, 0, 0};
  const char* label = "";
  switch (normalized) {
    case 'F':
      desired[0] = 1; desired[1] = 1; desired[2] = 1; desired[3] = 1;
      label = "Forward";
      break;
    case 'B':
      desired[0] = -1; desired[1] = -1; desired[2] = -1; desired[3] = -1;
      label = "Backward";
      break;
    case 'L':
      desired[0] = -1; desired[1] = 1; desired[2] = 1; desired[3] = -1;
      label = "Strafe Left";
      break;
    case 'R':
      desired[0] = 1; desired[1] = -1; desired[2] = -1; desired[3] = 1;
      label = "Strafe Right";
      break;
    case 'W':
      desired[0] = -1; desired[1] = 1; desired[2] = -1; desired[3] = 1;
      label = "Rotate Left";
      break;
    case 'U':
      desired[0] = 1; desired[1] = -1; desired[2] = 1; desired[3] = -1;
      label = "Rotate Right";
      break;
    default:
      return;
  }

  bool sameMotion = (activeMotionCmd == normalized && moving);
  if (sameMotion) {
    if (announce && normalized != lastReportedMotionCmd) {
      Serial.print(">> "); Serial.println(label);
      lastReportedMotionCmd = normalized;
    }
    return;
  }

  motorDir[0] = desired[0];
  motorDir[1] = desired[1];
  motorDir[2] = desired[2];
  motorDir[3] = desired[3];
  pidReset();
  targetSpeed = 0;
  calibrating = true;
  calibMeasuring = false;
  calibStartTime = millis();

  for (int i = 0; i < 4; i++) {
    driveMotor(i, motorDir[i], motorStartPwm(i));
  }
  moving = true;
  activeMotionCmd = normalized;
  if (announce) {
    Serial.print(">> "); Serial.println(label);
    lastReportedMotionCmd = normalized;
  }
}

int pulseToAxis(uint16_t pulse) {
  long centered = (long)pulse - 1500;
  if (abs(centered) <= RF_CENTER_DEADBAND_US) return 0;
  return (centered > 0) ? 1 : -1;
}

int pulseToSpeedTarget(uint16_t pulse) {
  if (pulse < 1300) return 180;
  if (pulse > 1700) return 320;
  return 250;
}

// Read ISR-captured pulse data into main-loop variables with glitch filtering
void readRfChannels() {
  unsigned long now = millis();
  uint16_t raw[RF_CHANNELS];
  bool hasNew[RF_CHANNELS];

  noInterrupts();
  for (int i = 0; i < RF_CHANNELS; i++) {
    raw[i] = rfPulseRaw[i];
    hasNew[i] = rfNewData[i];
    rfNewData[i] = false;
  }
  interrupts();

  for (int i = 0; i < RF_CHANNELS; i++) {
    if (hasNew[i]) {
      // Glitch filter: accept only if consistent with previous raw reading
      if (abs((int)raw[i] - (int)rfPulsePrev[i]) <= RF_GLITCH_THRESHOLD_US) {
        rfPulseUs[i] = raw[i];
        rfChanLastMs[i] = now;
      }
      rfPulsePrev[i] = raw[i];
    }
  }
}

bool isRfSignalFresh() {
  unsigned long now = millis();
  // CH5 (arm/disarm) must be fresh
  if ((now - rfChanLastMs[4]) > RF_SIGNAL_TIMEOUT_MS) return false;
  // At least one drive channel must be fresh
  for (int i = 0; i < 4; i++) {
    if ((now - rfChanLastMs[i]) <= RF_SIGNAL_TIMEOUT_MS) return true;
  }
  return false;
}

bool rfDriveInputsCentered() {
  return pulseToAxis(rfPulseUs[0]) == 0 &&
         pulseToAxis(rfPulseUs[1]) == 0 &&
         pulseToAxis(rfPulseUs[3]) == 0;
}

char getRfMotionCommand() {
  int rotate = pulseToAxis(rfPulseUs[0]);
  int throttle = -pulseToAxis(rfPulseUs[1]);
  int strafe = -pulseToAxis(rfPulseUs[3]);

  // Rotation takes priority when it's the only input
  if (rotate != 0) {
    return (rotate > 0) ? 'U' : 'W';
  }
  // When both throttle and strafe active, use raw deviation to pick dominant
  if (throttle != 0 && strafe != 0) {
    int tDev = abs((int)rfPulseUs[1] - 1500);
    int sDev = abs((int)rfPulseUs[3] - 1500);
    if (tDev > sDev) {
      return (throttle > 0) ? 'F' : 'B';
    }
    return (strafe > 0) ? 'L' : 'R';
  }
  if (throttle != 0) {
    return (throttle > 0) ? 'F' : 'B';
  }
  if (strafe != 0) {
    return (strafe > 0) ? 'L' : 'R';
  }
  return 'S';
}

void processRfControl() {
  readRfChannels();

  rfSignalPresent = isRfSignalFresh();
  if (!rfSignalPresent) {
    rfArmed = false;
    rfSeenDisarm = false;
    rfArmRequestStartMs = 0;
    if (rfLastSignalPresent) {
      Serial.println(">> RF signal lost");
    }
    rfLastSignalPresent = false;
    if (activeSource == SRC_RF) {
      applyStop(true);
      reportSource(SRC_NONE);
    }
    return;
  }

  rfLastSignalPresent = true;

  uint16_t armPulse = rfPulseUs[4];
  unsigned long nowMs = millis();

  if (armPulse < RF_ARM_LOW_US) {
    rfSeenDisarm = true;
    rfArmed = false;
    rfArmRequestStartMs = 0;
  } else if (armPulse > RF_ARM_HIGH_US) {
    if (!rfSeenDisarm) {
      rfArmed = false;
      rfArmRequestStartMs = 0;
    } else if (!rfDriveInputsCentered()) {
      rfArmed = false;
      rfArmRequestStartMs = 0;
    } else {
      if (rfArmRequestStartMs == 0) {
        rfArmRequestStartMs = nowMs;
      }
      if ((nowMs - rfArmRequestStartMs) >= RF_ARM_HOLD_MS) {
        rfArmed = true;
      }
    }
  } else {
    rfArmed = false;
    rfArmRequestStartMs = 0;
  }

  if (rfArmed != rfLastArmed) {
    Serial.println(rfArmed ? ">> RF armed" : ">> RF disarmed");
    rfLastArmed = rfArmed;
  }

  if (!rfArmed) {
    if (activeSource == SRC_RF) {
      applyStop(true);
      reportSource(SRC_NONE);
    }
    return;
  }

  reportSource(SRC_RF);
  setUserSpeedTarget(pulseToSpeedTarget(rfPulseUs[2]));
  applyMotionCommand(getRfMotionCommand());

  if (millis() - rfLastFramePrintMs >= 1000) {
    rfLastFramePrintMs = millis();
    Serial.print(">> RF frame ch1="); Serial.print(rfPulseUs[0]);
    Serial.print(" ch2="); Serial.print(rfPulseUs[1]);
    Serial.print(" ch3="); Serial.print(rfPulseUs[2]);
    Serial.print(" ch4="); Serial.print(rfPulseUs[3]);
    Serial.print(" ch5="); Serial.println(rfPulseUs[4]);
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
  char normalized = toupper(cmd);
  bool rfOwnsDrive = rfSignalPresent && rfArmed && activeSource == SRC_RF;

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
  switch (normalized) {
    case 'F':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      applyMotionCommand('F', true);
      break;
    case 'B':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      applyMotionCommand('B', true);
      break;
    case 'L':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      applyMotionCommand('L', true);
      break;
    case 'R':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      applyMotionCommand('R', true);
      break;
    case 'W':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      applyMotionCommand('W', true);
      break;
    case 'U':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      applyMotionCommand('U', true);
      break;
    case 'S':
      // Emergency stop always works — overrides RF for safety
      lastRosCmdMs = millis();
      if (rfOwnsDrive) {
        rfArmed = false;
        rfSeenDisarm = false;
        rfArmRequestStartMs = 0;
        Serial.println(">> ROS emergency stop - RF disarmed");
      }
      reportSource(SRC_ROS);
      applyStop(true);
      break;
    case '1':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      setUserSpeedTarget(180, true);
      break;
    case '2':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      setUserSpeedTarget(250, true);
      break;
    case '3':
      if (rfOwnsDrive) return;
      lastRosCmdMs = millis();
      reportSource(SRC_ROS);
      setUserSpeedTarget(320, true);
      break;
    case 'P':
      printEncoders();
      break;
    case 'C':
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

  // RF receiver pins with pull-up (pin stays HIGH when disconnected = safe)
  pinMode(RF_CH1_PIN, INPUT_PULLUP);
  pinMode(RF_CH2_PIN, INPUT_PULLUP);
  pinMode(RF_CH3_PIN, INPUT_PULLUP);
  pinMode(RF_CH4_PIN, INPUT_PULLUP);
  pinMode(RF_CH5_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(M1_CHA), isrM1, RISING);
  attachInterrupt(digitalPinToInterrupt(M2_CHA), isrM2, RISING);
  attachInterrupt(digitalPinToInterrupt(M3_CHA), isrM3, RISING);
  attachInterrupt(digitalPinToInterrupt(M4_CHA), isrM4, RISING);

  // Capture initial port states for PCINT edge detection
  lastPINB = PINB;
  lastPINK = PINK;

  // Enable pin-change interrupts for RF receiver
  PCMSK0 |= (1 << PCINT5) | (1 << PCINT6);       // PB5(D11), PB6(D12)
  PCMSK2 |= (1 << PCINT16) | (1 << PCINT17) | (1 << PCINT18);  // PK0(A8), PK1(A9), PK2(A10)
  PCICR  |= (1 << PCIE0) | (1 << PCIE2);          // Enable PCINT0 + PCINT2 vectors

  // Attach camera tilt servos and set to 0 (level)
  servoFront.attach(SERVO_FRONT_PIN);
  servoRear.attach(SERVO_REAR_PIN);
  servoFront.write(0);
  servoRear.write(0);

  stopAll();
  Serial.println("=== Mecanum PID + Servo Tilt ===");
  Serial.println("F/B/L/R/W/U/S  1=180 2=250 3=320  P=Ticks C=Clear");
  Serial.println("T<0-90>=Front tilt  G<0-90>=Rear tilt  CAL=Show/set cal");
  Serial.println("FlySky RF (PCINT): CH1=rot(D12) CH2=thr(D11) CH3=spd(A8) CH4=str(A9) CH5=arm(A10)");
  Serial.print(">> Motor cal: ");
  for (int i = 0; i < 4; i++) {
    Serial.print("M"); Serial.print(i + 1); Serial.print("="); Serial.print(motorCal[i], 2); Serial.print("("); Serial.print(motorStartPwm(i)); Serial.print(") ");
  }
  Serial.println();
}

void loop() {
  processRfControl();

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

  if (activeSource == SRC_ROS && !rfSignalPresent &&
      (millis() - lastRosCmdMs) > ROS_ACTIVITY_TIMEOUT_MS &&
      activeMotionCmd == 'S') {
    reportSource(SRC_NONE);
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
