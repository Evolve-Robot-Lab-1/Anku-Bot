#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <util/atomic.h>

// M1-M3 use A-edge x2 decoding with B sampled inside each ISR. M4 A on D17
// is polled because D17 has no Mega external interrupt. Counts are raw ticks.
struct MotorPins { uint8_t in1, in2, pwm, a, b; };
const MotorPins pins[4] = {
  {24, 25, 11, 19, 18}, {22, 23, 10, 2, 35},
  {26, 27, 9, 3, 37}, {28, 29, 8, 17, 39}
};
Adafruit_PWMServoDriver pca(0x41);
const uint8_t PCA_OE_PIN = 7;
const uint8_t SERVO_STEP_MS = 20;
const uint16_t SERVO_LEG_MS[2] = {1500, 2500}; // Shoulder uses the slower second duration.
const uint8_t SHOULDER_CHANNEL = 2;
const uint16_t CH0_START_US = 1000;
const uint16_t CH0_END_US = 1333;
// Start one PCA9685 count below the known 1000 us hold.
const uint8_t SHOULDER_MAX_DEGREES = 1;
const uint16_t SHOULDER_DOWN_US = 995;
const uint16_t SHOULDER_UP_US = 1000;
// Relative jog step: one PCA9685 count at 50 Hz is ~4.88 us. Keep single-step
// motion under ~1 nominal degree so each E/T press is barely visible.
const int8_t SHOULDER_JOG_STEP_US = 5;
const uint16_t SHOULDER_MIN_US = 990;
const uint16_t SHOULDER_MAX_US = 1100;
struct EncoderData {
  int32_t ticks;
  uint32_t edges, bHigh, bLow;
};
volatile EncoderData encoder[4] = {};
const uint8_t TEST_PWM = 200;
const uint32_t PULSE_MS = 1000;
const uint32_t REPORT_MS = 250;
bool running = false;
bool pcaFound = false;
bool servoTesting = false;
uint8_t servoChannel = 0;
uint16_t servoStartUs = 1000;
uint16_t servoEndUs = 1333;
uint16_t servoLegMs = 1500;
uint32_t servoStepAt = 0;
uint32_t servoStartedAt = 0;
bool shoulderMoving = false;
bool shoulderHolding = false;
uint16_t shoulderPulseUs = SHOULDER_DOWN_US;
uint16_t shoulderStartUs = SHOULDER_DOWN_US;
uint16_t shoulderTargetUs = SHOULDER_DOWN_US;
uint32_t shoulderStartedAt = 0;
uint32_t shoulderStepAt = 0;
bool gripperTesting = false;
uint8_t gripperPhase = 0;
uint32_t gripperPhaseAt = 0;
const uint16_t GRIPPER_PULSES_US[3][2] = {
  {1500, 2000}, // Valve neutral, pump running.
  {2000, 1500}, // Valve actuated, pump neutral.
  {1500, 1500}  // Both neutral before disabling outputs.
};
const uint16_t GRIPPER_PHASE_MS[3] = {3000, 1000, 250};
uint32_t startedAt = 0, lastReport = 0;
bool motor4LastA = false;
char command[24];
uint8_t commandLength = 0;
bool overflow = false;

void recordEdge(uint8_t i) {
  bool a = digitalRead(pins[i].a);
  bool b = digitalRead(pins[i].b);
  encoder[i].ticks += a == b ? 1 : -1;
  ++encoder[i].edges;
  if (b) ++encoder[i].bHigh;
  else ++encoder[i].bLow;
}
void encoder1ISR() { recordEdge(0); }
void encoder2ISR() { recordEdge(1); }
void encoder3ISR() { recordEdge(2); }

// Mega D17 has no external-interrupt input, so sample M4 A in loop().
void pollMotor4Encoder() {
  bool a = digitalRead(pins[3].a);
  if (a != motor4LastA) {
    motor4LastA = a;
    recordEdge(3);
  }
}

void disablePcaOutputs() {
  digitalWrite(PCA_OE_PIN, HIGH);
  servoTesting = false;
  shoulderMoving = false;
  shoulderHolding = false;
  shoulderPulseUs = SHOULDER_DOWN_US;
  gripperTesting = false;
  if (pcaFound) {
    for (uint8_t ch = 0; ch < 16; ++ch) pca.setPWM(ch, 0, 4096);
  }
}

void startServoSweep(uint8_t channel) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (running || servoTesting || gripperTesting || shoulderMoving || shoulderHolding) { Serial.println(F("ERROR,BUSY")); return; }
  disablePcaOutputs();
  servoTesting = true;
  servoChannel = channel;
  servoStartUs = channel == SHOULDER_CHANNEL ? SHOULDER_DOWN_US : CH0_START_US;
  servoEndUs = channel == SHOULDER_CHANNEL ? SHOULDER_UP_US : CH0_END_US;
  servoLegMs = channel == SHOULDER_CHANNEL ? SERVO_LEG_MS[1] : SERVO_LEG_MS[0];
  servoStepAt = millis() - SERVO_STEP_MS;
  servoStartedAt = millis();
  Serial.print(F("SERVO_START,CH")); Serial.print(servoChannel);
  if (channel == SHOULDER_CHANNEL) {
    Serial.print(F(",0_TO_1_TO_0,"));
  } else Serial.print(',');
  Serial.print(servoStartUs); Serial.print(F("_TO_"));
  Serial.print(servoEndUs); Serial.println(F("_TO_START_US"));
}

void startShoulder(uint16_t target) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (running || servoTesting || gripperTesting || shoulderMoving) { Serial.println(F("ERROR,BUSY")); return; }
  // After a serial reset, the controller loses its commanded position. Permit
  // the explicit lower command to start from the raised endpoint.
  if (target == SHOULDER_DOWN_US && !shoulderHolding) shoulderPulseUs = SHOULDER_UP_US;
  if (shoulderHolding && shoulderPulseUs == target) {
    Serial.print(F("SERVO_HOLDING,CH2,")); Serial.println(target);
    return;
  }
  if (!shoulderHolding) {
    const uint16_t startUs = shoulderPulseUs;
    disablePcaOutputs();
    shoulderPulseUs = startUs;
  }
  shoulderStartUs = shoulderPulseUs;
  shoulderTargetUs = target;
  shoulderMoving = true;
  shoulderHolding = false;
  shoulderStartedAt = millis();
  shoulderStepAt = millis() - SERVO_STEP_MS;
  Serial.print(F("SERVO_START,CH2,")); Serial.print(shoulderStartUs);
  Serial.print(F("_TO_")); Serial.println(shoulderTargetUs);
}

// Relative jog from the actively held position: no output disable, no
// absolute assumption, so repeated E/T presses stay tiny and smooth. The
// first enable after a reset/flash still snaps to the tracked pulse because
// the servo position is unknown while outputs are disabled; keep the arm
// supported for that single capture, then use E/T only.
void jogShoulder(int8_t stepUs) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (running || servoTesting || gripperTesting || shoulderMoving) { Serial.println(F("ERROR,BUSY")); return; }
  if (!shoulderHolding) { Serial.println(F("ERROR,NOT_HOLDING_SEND_B_OR_C_FIRST")); return; }
  int16_t target = int16_t(shoulderPulseUs) + stepUs;
  if (target < int16_t(SHOULDER_MIN_US) || target > int16_t(SHOULDER_MAX_US)) {
    Serial.print(F("ERROR,LIMIT,")); Serial.println(shoulderPulseUs);
    return;
  }
  shoulderStartUs = shoulderPulseUs;
  shoulderTargetUs = uint16_t(target);
  shoulderMoving = true;
  shoulderHolding = false;
  shoulderStartedAt = millis();
  shoulderStepAt = millis() - SERVO_STEP_MS;
  Serial.print(F("SERVO_START,CH2,")); Serial.print(shoulderStartUs);
  Serial.print(F("_TO_")); Serial.println(shoulderTargetUs);
}

void serviceShoulder() {
  if (!shoulderMoving || uint32_t(millis() - shoulderStepAt) < SERVO_STEP_MS) return;
  shoulderStepAt = millis();
  const uint32_t elapsed = min(uint32_t(millis() - shoulderStartedAt), uint32_t(SERVO_LEG_MS[1]));
  const uint32_t t = elapsed * 1024UL / SERVO_LEG_MS[1];
  const uint32_t eased = t * t * (3072UL - 2UL * t) / (1024UL * 1024UL);
  const int32_t pulseQ = int32_t(shoulderStartUs) * 1024L
    + (int32_t(shoulderTargetUs) - shoulderStartUs) * int32_t(eased);
  const uint16_t us = uint16_t(pulseQ / 1024L);
  pca.setPWM(SHOULDER_CHANNEL, 0, uint32_t(us) * 4096UL / 20000UL);
  digitalWrite(PCA_OE_PIN, LOW);
  if (elapsed >= SERVO_LEG_MS[1]) {
    shoulderMoving = false;
    shoulderHolding = true;
    shoulderPulseUs = shoulderTargetUs;
    Serial.print(F("SERVO_HOLDING,CH2,")); Serial.println(shoulderPulseUs);
  }
}

void serviceServoSweep() {
  if (!servoTesting || uint32_t(millis() - servoStepAt) < SERVO_STEP_MS) return;
  servoStepAt = millis();
  const uint32_t elapsed = uint32_t(millis() - servoStartedAt);
  const bool returning = elapsed >= servoLegMs;
  const uint32_t legElapsed = returning ? min(elapsed - servoLegMs, uint32_t(servoLegMs)) : elapsed;
  const uint32_t t = legElapsed * 1024UL / servoLegMs;
  // Cubic smoothstep: zero velocity at each end and at completion.
  const uint32_t eased = t * t * (3072UL - 2UL * t) / (1024UL * 1024UL);
  const uint16_t fromUs = returning ? servoEndUs : servoStartUs;
  const uint16_t toUs = returning ? servoStartUs : servoEndUs;
  const int32_t pulseQ = int32_t(fromUs) * 1024L + (int32_t(toUs) - fromUs) * int32_t(eased);
  const uint16_t us = uint16_t(pulseQ / 1024L);
  pca.setPWM(servoChannel, 0, (uint32_t)us * 4096UL / 20000UL);
  digitalWrite(PCA_OE_PIN, LOW);
  if (elapsed >= 2UL * servoLegMs) {
    servoTesting = false;
    Serial.print(F("SERVO_CYCLE_COMPLETE,CH")); Serial.println(servoChannel);
    if (servoChannel == SHOULDER_CHANNEL) {
      // Keep the shoulder at the lower pulse after the sweep so it does not
      // lose holding torque and sag as soon as the diagnostic ends.
      shoulderPulseUs = servoStartUs;
      shoulderHolding = true;
      Serial.print(F("SERVO_HOLDING,CH2,")); Serial.println(shoulderPulseUs);
    } else disablePcaOutputs();
  }
}

void applyGripperPhase() {
  pca.setPWM(6, 0, (uint32_t)GRIPPER_PULSES_US[gripperPhase][0] * 4096UL / 20000UL);
  pca.setPWM(7, 0, (uint32_t)GRIPPER_PULSES_US[gripperPhase][1] * 4096UL / 20000UL);
  digitalWrite(PCA_OE_PIN, LOW);
  gripperPhaseAt = millis();
  Serial.print(F("GRIPPER_PHASE,")); Serial.print(gripperPhase);
  Serial.print(F(",MS,")); Serial.println(GRIPPER_PHASE_MS[gripperPhase]);
}

void startGripperTest() {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (running || servoTesting || gripperTesting || shoulderMoving || shoulderHolding) { Serial.println(F("ERROR,BUSY")); return; }
  disablePcaOutputs();
  gripperTesting = true;
  gripperPhase = 0;
  Serial.println(F("GRIPPER_START,CH6_VALVE_CH7_PUMP"));
  applyGripperPhase();
}

void serviceGripperTest() {
  if (!gripperTesting || uint32_t(millis() - gripperPhaseAt) < GRIPPER_PHASE_MS[gripperPhase]) return;
  if (++gripperPhase < 3) applyGripperPhase();
  else {
    disablePcaOutputs();
    Serial.println(F("GRIPPER_CYCLE_COMPLETE"));
  }
}

void scanPca9685() {
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(25000UL, true);
  bool expectedFound = false;
  uint8_t foundCount = 0;
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      ++foundCount;
      Serial.print(F("I2C,FOUND,0x")); Serial.println(address, HEX);
      if (address == 0x41) expectedFound = true;
    }
  }
  if (expectedFound) {
    Serial.println(F("PCA9685,FOUND,0x41"));
    pca.begin();
    pca.setPWMFreq(50);
    pcaFound = true;
    disablePcaOutputs();
  } else Serial.println(F("PCA9685,NOT_FOUND,EXPECTED_0x41"));
  if (!foundCount) Serial.println(F("I2C,NO_DEVICES"));
}

void stopAll() {
  for (uint8_t i = 0; i < 4; ++i) {
    analogWrite(pins[i].pwm, 0);
    digitalWrite(pins[i].in1, LOW);
    digitalWrite(pins[i].in2, LOW);
  }
  running = false;
  disablePcaOutputs();
}

void reportEncoders() {
  EncoderData snapshot[4];
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    for (uint8_t i = 0; i < 4; ++i) {
      snapshot[i].ticks = encoder[i].ticks;
      snapshot[i].edges = encoder[i].edges;
      snapshot[i].bHigh = encoder[i].bHigh;
      snapshot[i].bLow = encoder[i].bLow;
    }
  }
  for (uint8_t i = 0; i < 4; ++i) {
    Serial.print(F("ENC,M")); Serial.print(i + 1);
    Serial.print(F(",TICKS,")); Serial.print(snapshot[i].ticks);
    Serial.print(F(",A_EDGES,")); Serial.print(snapshot[i].edges);
    Serial.print(F(",B_HIGH_AT_A,")); Serial.print(snapshot[i].bHigh);
    Serial.print(F(",B_LOW_AT_A,")); Serial.print(snapshot[i].bLow);
    Serial.print(F(",A,")); Serial.print(digitalRead(pins[i].a));
    Serial.print(F(",B,")); Serial.print(digitalRead(pins[i].b));
    Serial.print(F(",SIGNAL,"));
    if (!snapshot[i].edges) Serial.println(F("NO_A_EDGES"));
    else if (!snapshot[i].bLow) Serial.println(F("B_ALWAYS_HIGH_AT_A"));
    else if (!snapshot[i].bHigh) Serial.println(F("B_ALWAYS_LOW_AT_A"));
    else Serial.println(F("BOTH_B_LEVELS_SEEN"));
  }
}

void resetEncoders() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    for (uint8_t i = 0; i < 4; ++i) {
      encoder[i].ticks = 0; encoder[i].edges = 0;
      encoder[i].bHigh = 0; encoder[i].bLow = 0;
    }
  }
}

void startPulse(uint8_t mask, bool forward) {
  if (running || servoTesting || gripperTesting || shoulderMoving || shoulderHolding) { Serial.println(F("ERROR,BUSY")); return; }
  stopAll();
  startedAt = millis();
  running = true;
  for (uint8_t i = 0; i < 4; ++i) {
    if (mask & (1 << i)) {
      digitalWrite(pins[i].in1, forward ? HIGH : LOW);
      digitalWrite(pins[i].in2, forward ? LOW : HIGH);
      analogWrite(pins[i].pwm, TEST_PWM);
    }
  }
  Serial.print(F("START,MASK,")); Serial.print(mask);
  Serial.println(forward ? F(",F,PWM200,1000MS") : F(",R,PWM200,1000MS"));
}

void executeCommand() {
  command[commandLength] = '\0';
  if (overflow) { stopAll(); Serial.println(F("ERROR,COMMAND_TOO_LONG")); }
  else if (!strcmp(command, "STOP") || !strcmp(command, "S")) {
    stopAll(); Serial.println(F("STOPPED"));
  } else if (!strcmp(command, "STATUS") || !strcmp(command, "ENC")) {
    Serial.println((running || servoTesting || gripperTesting || shoulderMoving) ? F("RUNNING") : shoulderHolding ? F("HOLDING,CH2") : F("STOPPED")); reportEncoders();
  } else if (!strcmp(command, "A")) {
    startServoSweep(0);
  } else if (!strcmp(command, "B")) {
    startShoulder(SHOULDER_UP_US);
  } else if (!strcmp(command, "C")) {
    startShoulder(SHOULDER_DOWN_US);
  } else if (!strcmp(command, "E")) {
    jogShoulder(-SHOULDER_JOG_STEP_US);
  } else if (!strcmp(command, "T")) {
    jogShoulder(SHOULDER_JOG_STEP_US);
  } else if (!strcmp(command, "D")) {
    startServoSweep(SHOULDER_CHANNEL);
  } else if (!strcmp(command, "G")) {
    startGripperTest();
  } else if (!strcmp(command, "RST")) {
    if (running || servoTesting || gripperTesting || shoulderMoving) Serial.println(F("ERROR,BUSY"));
    else { resetEncoders(); Serial.println(F("RESET")); reportEncoders(); }
  } else if (!strcmp(command, "F")) startPulse(15, true);
  else if (!strcmp(command, "R")) startPulse(15, false);
  else if (!strcmp(command, "M1F")) startPulse(1, true);
  else if (!strcmp(command, "M1R")) startPulse(1, false);
  else if (!strcmp(command, "M2F")) startPulse(2, true);
  else if (!strcmp(command, "M2R")) startPulse(2, false);
  else if (!strcmp(command, "M3F")) startPulse(4, true);
  else if (!strcmp(command, "M3R")) startPulse(4, false);
  else if (!strcmp(command, "M4F")) startPulse(8, true);
  else if (!strcmp(command, "M4R")) startPulse(8, false);
  else if (commandLength) { stopAll(); Serial.println(F("ERROR,UNKNOWN_COMMAND")); }
  commandLength = 0; overflow = false;
}

void setup() {
  digitalWrite(PCA_OE_PIN, HIGH);
  pinMode(PCA_OE_PIN, OUTPUT);
  const uint8_t pwmPins[] = {8, 9, 10, 11, 12};
  for (uint8_t p : pwmPins) { digitalWrite(p, LOW); pinMode(p, OUTPUT); analogWrite(p, 0); }
  for (uint8_t p = 22; p <= 29; ++p) { digitalWrite(p, LOW); pinMode(p, OUTPUT); }
  stopAll();
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(pins[i].a, INPUT_PULLUP); pinMode(pins[i].b, INPUT_PULLUP);
  }
  motor4LastA = digitalRead(pins[3].a);
  attachInterrupt(digitalPinToInterrupt(pins[0].a), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pins[1].a), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pins[2].a), encoder3ISR, CHANGE);
  Serial.begin(115200);
  scanPca9685();
  Serial.println(F("READY,FOUR_MOTOR_PCA_ARM,M1=24/25/11,A19/B18,M2=22/23/10,A2/B35,M3=26/27/9,A3/B37,M4=28/29/8,A17_POLLED/B39"));
  Serial.println(F("A: CH0 smooth sweep; D: CH2 one-count pulse check; B/C: CH2 one-count move/hold; E/T: CH2 -5/+5us jog from hold; G: gripper; F/R or M1F..M4R motors; STOP releases outputs."));
}

void loop() {
  pollMotor4Encoder();
  serviceServoSweep();
  serviceShoulder();
  serviceGripperTest();
  if (running && uint32_t(millis() - startedAt) >= PULSE_MS) {
    stopAll(); Serial.println(F("STOPPED,TIMEOUT")); reportEncoders();
    lastReport = millis();
  }
  // No telemetry writes while running: serial backpressure must not delay stop.
  if (!running && !servoTesting && !gripperTesting && !shoulderMoving && uint32_t(millis() - lastReport) >= REPORT_MS) {
    lastReport = millis(); reportEncoders();
  }
  for (uint8_t n = 0; n < 16 && Serial.available(); ++n) {
    char c = Serial.read();
    if (c == '\n') executeCommand();
    else if (c != '\r') {
      if (commandLength < sizeof(command) - 1) command[commandLength++] = c;
      else overflow = true;
    }
  }
}
