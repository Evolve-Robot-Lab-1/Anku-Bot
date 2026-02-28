#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Clean CH0-only servo sweep for Bot 1.
// Servo 2 is plugged on PCA CH0. No other servo is driven.
// PCA9685 @ 0x41, OE on Mega D7 (active low).
// Sweep: 1000 -> 1083 us (~15 deg) -> 1000, 2.5 s per leg, cubic ease.
// Holds the start pulse after completion so the joint does not sag.
// STOP disables all PCA outputs (joint goes slack, keep arm supported).

Adafruit_PWMServoDriver pca(0x41);
const uint8_t PCA_OE_PIN = 7;
const uint8_t CH0 = 0;
const uint8_t CH1 = 1;
const uint8_t CH11 = 11;
const uint8_t CH_VALVE = 6;
const uint8_t CH_PUMP = 7;
// Gripper phases (from bench history): suction = valve neutral + pump on;
// release = valve actuated + pump neutral.
const uint16_t SUCTION_US[2] = {1500, 2100};
const uint16_t RELEASE_US[2] = {2000, 1500};
const uint16_t START_US = 2000;
const uint16_t END_US = 1917;
const uint16_t LEG_MS = 2500;
const uint8_t STEP_MS = 20;

bool pcaFound = false;
bool sweeping = false;
bool holding[16] = {false};
uint16_t holdUs[16] = {0};
uint8_t gripMode = 0; // 0 off, 1 suction, 2 release
uint8_t swCh = 0;
uint16_t swStartUs = START_US;
uint16_t swEndUs = END_US;
uint16_t swLegMs = LEG_MS;
uint32_t startedAt = 0;
uint32_t stepAt = 0;
char cmd[24];
uint8_t cmdLen = 0;
bool overflow = false;

static uint16_t usToCounts(uint16_t us) {
  return uint32_t(us) * 4096UL / 20000UL;
}

void disableOutputs() {
  digitalWrite(PCA_OE_PIN, HIGH);
  sweeping = false;
  for (uint8_t i = 0; i < 16; ++i) holding[i] = false;
  gripMode = 0;
  if (pcaFound) {
    for (uint8_t ch = 0; ch < 16; ++ch) pca.setPWM(ch, 0, 4096);
  }
}

// Keep the other channel's hold alive while driving one channel: rewrite
// both latched pulses, then enable outputs. Full-off would drop the arm.
void writeHolds() {
  if (pcaFound) {
    for (uint8_t i = 0; i < 16; ++i) {
      if (holding[i]) pca.setPWM(i, 0, usToCounts(holdUs[i]));
    }
    if (gripMode == 1) {
      pca.setPWM(CH_VALVE, 0, usToCounts(SUCTION_US[0]));
      pca.setPWM(CH_PUMP, 0, usToCounts(SUCTION_US[1]));
    } else if (gripMode == 2) {
      pca.setPWM(CH_VALVE, 0, usToCounts(RELEASE_US[0]));
      pca.setPWM(CH_PUMP, 0, usToCounts(RELEASE_US[1]));
    }
  }
  digitalWrite(PCA_OE_PIN, LOW);
}

void setGrip(uint8_t mode) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  gripMode = mode;
  writeHolds();
  Serial.println(mode == 1 ? F("GRIP,SUCTION_ON") : F("GRIP,RELEASE"));
}

void gotoUs(uint8_t ch, uint16_t target) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 15) { Serial.println(F("ERROR,CHANNEL")); return; }
  if (target < 900 || target > 2250) { Serial.println(F("ERROR,RANGE_900_2250")); return; }
  sweeping = false;
  holding[ch] = true;
  holdUs[ch] = target;
  writeHolds();
  Serial.print(F("SERVO_HOLDING,CH")); Serial.print(ch); Serial.print(F(","));
  Serial.println(holdUs[ch]);
}

void gotoZero() {
  gotoUs(CH0, START_US);
}

// One-shot dual poses (work from STOPPED, no prior hold needed).
void gotoPose(uint16_t ch0us, uint16_t ch1us) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch0us < 900 || ch0us > 2250 || ch1us < 900 || ch1us > 2250) {
    Serial.println(F("ERROR,RANGE_900_2250")); return;
  }
  sweeping = false;
  holding[0] = holding[1] = true;
  holdUs[0] = ch0us;
  holdUs[1] = ch1us;
  writeHolds();
  Serial.print(F("POSE,CH0,")); Serial.print(holdUs[0]);
  Serial.print(F(",CH1,")); Serial.println(holdUs[1]);
}

// Rest pose: CH0 210 (2165) + CH1 60 (1335) + CH11 60 (1335).
void gotoRest() {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  sweeping = false;
  holding[0] = holding[1] = holding[11] = true;
  holdUs[0] = 2165;
  holdUs[1] = 1335;
  holdUs[11] = 1335;
  writeHolds();
  Serial.println(F("POSE,REST,CH0,2165,CH1,1335,CH11,1335"));
}

// Carry-all: CH0 220 + CH1 60 + CH11 0 in one command (reset-proof restore).
void gotoCarry() {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  sweeping = false;
  holding[0] = holding[1] = holding[11] = true;
  holdUs[0] = 2220;
  holdUs[1] = 1335;
  holdUs[11] = 1000;
  writeHolds();
  Serial.println(F("POSE,CARRY,CH0,2220,CH1,1335,CH11,1000"));
}

void jog(uint8_t ch, int8_t stepUs) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 15) { Serial.println(F("ERROR,CHANNEL")); return; }
  if (!holding[ch]) { Serial.println(F("ERROR,NOT_HOLDING")); return; }
  int16_t target = int16_t(holdUs[ch]) + stepUs;
  if (target < 900 || target > 2250) {
    Serial.print(F("ERROR,LIMIT,")); Serial.println(holdUs[ch]);
    return;
  }
  holdUs[ch] = uint16_t(target);
  writeHolds();
  Serial.print(F("SERVO_HOLDING,CH")); Serial.print(ch); Serial.print(F(","));
  Serial.println(holdUs[ch]);
}
void startSweep(uint8_t ch) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 15) { Serial.println(F("ERROR,CHANNEL")); return; }
  sweeping = true;
  holding[ch] = false;
  swCh = ch;
  swStartUs = START_US;
  swEndUs = END_US;
  swLegMs = LEG_MS;
  startedAt = millis();
  stepAt = millis() - STEP_MS;
  Serial.print(F("SERVO_START,CH")); Serial.print(swCh); Serial.print(F(","));
  Serial.print(swStartUs); Serial.print(F("_TO_"));
  Serial.print(swEndUs); Serial.println(F("_TO_START_US"));
}

// Working window sweep: 200 deg (2110 us) <-> 220 deg (2220 us), holds low end.
void startWindowSweep(uint8_t ch) {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (ch > 15) { Serial.println(F("ERROR,CHANNEL")); return; }
  sweeping = true;
  holding[ch] = false;
  swCh = ch;
  swStartUs = 2110;
  swEndUs = 2220;
  swLegMs = LEG_MS;
  startedAt = millis();
  stepAt = millis() - STEP_MS;
  Serial.print(F("SERVO_START,CH")); Serial.print(swCh);
  Serial.println(F(",WIN200_TO_220"));
}

// Slow sweep: CH0 220 flat-lock (2220) <-> 210 rise (2165), 5 s/leg.
// Requires CH1 already holding (stays locked at its pulse throughout).
void startSlowSweep() {
  if (!pcaFound) { Serial.println(F("ERROR,PCA_NOT_FOUND")); return; }
  if (sweeping) { Serial.println(F("ERROR,BUSY")); return; }
  if (!holding[1]) { Serial.println(F("ERROR,CH1_NOT_HOLDING")); return; }
  sweeping = true;
  holding[0] = false;
  swCh = 0;
  swStartUs = 2220;
  swEndUs = 2165;
  swLegMs = 5000;
  startedAt = millis();
  stepAt = millis() - STEP_MS;
  Serial.println(F("SERVO_START,CH0,SLOW220_TO_210"));
}

void serviceSweep() {
  if (!sweeping) return;
  if (uint32_t(millis() - stepAt) < STEP_MS) return;
  stepAt = millis();
  uint32_t elapsed = uint32_t(millis() - startedAt);
  bool returning = elapsed >= swLegMs;
  uint32_t legElapsed = returning ? elapsed - swLegMs : elapsed;
  if (legElapsed > swLegMs) legElapsed = swLegMs;
  uint32_t t = legElapsed * 1024UL / swLegMs;
  uint32_t eased = t * t * (3072UL - 2UL * t) / (1024UL * 1024UL);
  uint16_t fromUs = returning ? swEndUs : swStartUs;
  uint16_t toUs = returning ? swStartUs : swEndUs;
  int32_t q = int32_t(fromUs) * 1024L + (int32_t(toUs) - fromUs) * int32_t(eased);
  pca.setPWM(swCh, 0, usToCounts(uint16_t(q / 1024L)));
  for (uint8_t i = 0; i < 16; ++i) {
    if (i != swCh && holding[i]) pca.setPWM(i, 0, usToCounts(holdUs[i]));
  }
  digitalWrite(PCA_OE_PIN, LOW);
  if (elapsed >= 2UL * swLegMs) {
    sweeping = false;
    holding[swCh] = true;
    holdUs[swCh] = swStartUs;
    writeHolds();
    Serial.print(F("SERVO_CYCLE_COMPLETE,CH")); Serial.println(swCh);
    Serial.print(F("SERVO_HOLDING,CH")); Serial.print(swCh); Serial.print(F(","));
    Serial.println(holdUs[swCh]);
  }
}

void execCmd() {
  cmd[cmdLen] = '\0';
  if (overflow) { disableOutputs(); Serial.println(F("ERROR,COMMAND_TOO_LONG")); }
  else if (!strcmp(cmd, "A")) startSweep(0);
  else if (!strcmp(cmd, "W")) startWindowSweep(0);
  else if (!strcmp(cmd, "Z")) gotoZero();
  else if (!strcmp(cmd, "E")) jog(0, -5);
  else if (!strcmp(cmd, "T")) jog(0, 5);
  else if (!strcmp(cmd, "N0")) gotoUs(0, 1000);
  else if (!strcmp(cmd, "N90")) gotoUs(0, 1500);
  else if (!strcmp(cmd, "N180")) gotoUs(0, 2000);
  else if (!strcmp(cmd, "B")) startWindowSweep(1);
  else if (!strcmp(cmd, "X")) gotoUs(1, 2000);
  else if (!strcmp(cmd, "Y")) jog(1, -5);
  else if (!strcmp(cmd, "U")) jog(1, 5);
  else if (!strcmp(cmd, "M0")) gotoUs(1, 1000);
  else if (!strcmp(cmd, "M90")) gotoUs(1, 1500);
  else if (!strcmp(cmd, "M180")) gotoUs(1, 2000);
  else if (!strcmp(cmd, "Q")) gotoPose(2110, 1335);
  else if (!strcmp(cmd, "P")) gotoPose(2220, 1335);
  else if (!strcmp(cmd, "R")) gotoPose(2165, 1335);
  else if (!strcmp(cmd, "C")) gotoCarry();
  else if (!strcmp(cmd, "F")) gotoRest();
  else if (!strcmp(cmd, "V")) startSlowSweep();
  else if (!strcmp(cmd, "O")) setGrip(1);
  else if (!strcmp(cmd, "L")) setGrip(2);
  else if (cmd[0] == 'G') {
    // Generic setter: G<ch>,<us> e.g. G11,1335. Keeps other holds alive.
    int ch = -1, us = -1;
    if (sscanf((const char*)(cmd + 1), "%d,%d", &ch, &us) == 2) {
      if (ch >= 0 && ch < 16) gotoUs((uint8_t)ch, (uint16_t)us);
      else Serial.println(F("ERROR,CHANNEL"));
    } else Serial.println(F("ERROR,USE_Gch,us"));
  }
  else if (!strcmp(cmd, "D")) startWindowSweep(11);
  else if (!strcmp(cmd, "H")) gotoUs(11, 2000);
  else if (!strcmp(cmd, "J")) jog(11, -5);
  else if (!strcmp(cmd, "K")) jog(11, 5);
  else if (!strcmp(cmd, "H0")) gotoUs(11, 1000);
  else if (!strcmp(cmd, "H90")) gotoUs(11, 1500);
  else if (!strcmp(cmd, "H180")) gotoUs(11, 2000);
  else if (!strcmp(cmd, "STOP") || !strcmp(cmd, "S")) { disableOutputs(); Serial.println(F("STOPPED")); }
  else if (!strcmp(cmd, "STATUS")) {
    if (sweeping) Serial.println(F("RUNNING"));
    else {
      Serial.print(F("HOLDING,CH0,")); Serial.print(holding[0] ? holdUs[0] : 0);
      Serial.print(F(",CH1,")); Serial.print(holding[1] ? holdUs[1] : 0);
      Serial.print(F(",CH11,")); Serial.println(holding[11] ? holdUs[11] : 0);
    }
  }
  else if (cmdLen) { Serial.println(F("ERROR,UNKNOWN_COMMAND")); }
  cmdLen = 0; overflow = false;
}

void setup() {
  digitalWrite(PCA_OE_PIN, HIGH);
  pinMode(PCA_OE_PIN, OUTPUT);
  // Keep base motors unpowered in this servo-only build.
  const uint8_t pwmPins[] = {8, 9, 10, 11, 12};
  for (uint8_t p : pwmPins) { digitalWrite(p, LOW); pinMode(p, OUTPUT); analogWrite(p, 0); }
  for (uint8_t p = 22; p <= 29; ++p) { digitalWrite(p, LOW); pinMode(p, OUTPUT); }
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  Wire.setWireTimeout(25000UL, true);
  bool found = false;
  for (uint8_t a = 0x08; a <= 0x77; ++a) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("I2C,FOUND,0x")); Serial.println(a, HEX);
      if (a == 0x41) found = true;
    }
  }
  if (found) {
    Serial.println(F("PCA9685,FOUND,0x41"));
    pca.begin();
    pca.setPWMFreq(50);
    pcaFound = true;
    disableOutputs();
  } else {
    Serial.println(F("PCA9685,NOT_FOUND,EXPECTED_0x41"));
  }
  Serial.println(F("READY,CH0_CH1_DUAL"));
  Serial.println(F("CH0: W Z N E/T | CH1: B X M Y/U | CH11: D H J/K | Q/R/P/C/F V O/L | Gch,us STOP STATUS"));
}

void loop() {
  serviceSweep();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') execCmd();
    else if (c != '\r') {
      if (cmdLen < sizeof(cmd) - 1) cmd[cmdLen++] = c;
      else overflow = true;
    }
  }
}
