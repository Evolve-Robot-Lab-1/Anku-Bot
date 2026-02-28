#include <Arduino.h>
#include <util/atomic.h>

// A-edge x2 decoding: B is sampled inside the ISR, not inferred from
// occasional serial snapshots. Counts are not calibrated revolutions.
struct MotorPins { uint8_t in1, in2, pwm, a, b; };
const MotorPins pins[2] = {{24, 25, 11, 19, 18}, {22, 23, 10, 2, 35}};
struct EncoderData {
  int32_t ticks;
  uint32_t edges, bHigh, bLow;
};
volatile EncoderData encoder[2] = {};
const uint8_t TEST_PWM = 200;
const uint32_t PULSE_MS = 1000;
const uint32_t REPORT_MS = 250;
bool running = false;
uint32_t startedAt = 0, lastReport = 0;
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

void stopAll() {
  for (uint8_t i = 0; i < 2; ++i) {
    analogWrite(pins[i].pwm, 0);
    digitalWrite(pins[i].in1, LOW);
    digitalWrite(pins[i].in2, LOW);
  }
  running = false;
}

void reportEncoders() {
  EncoderData snapshot[2];
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    for (uint8_t i = 0; i < 2; ++i) {
      snapshot[i].ticks = encoder[i].ticks;
      snapshot[i].edges = encoder[i].edges;
      snapshot[i].bHigh = encoder[i].bHigh;
      snapshot[i].bLow = encoder[i].bLow;
    }
  }
  for (uint8_t i = 0; i < 2; ++i) {
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
    for (uint8_t i = 0; i < 2; ++i) {
      encoder[i].ticks = 0; encoder[i].edges = 0;
      encoder[i].bHigh = 0; encoder[i].bLow = 0;
    }
  }
}

void startPulse(uint8_t mask, bool forward) {
  if (running) { Serial.println(F("ERROR,BUSY")); return; }
  stopAll();
  startedAt = millis();
  running = true;
  for (uint8_t i = 0; i < 2; ++i) {
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
    Serial.println(running ? F("RUNNING") : F("STOPPED")); reportEncoders();
  } else if (!strcmp(command, "RST")) {
    if (running) Serial.println(F("ERROR,BUSY"));
    else { resetEncoders(); Serial.println(F("RESET")); reportEncoders(); }
  } else if (!strcmp(command, "F")) startPulse(3, true);
  else if (!strcmp(command, "R")) startPulse(3, false);
  else if (!strcmp(command, "M1F")) startPulse(1, true);
  else if (!strcmp(command, "M1R")) startPulse(1, false);
  else if (!strcmp(command, "M2F")) startPulse(2, true);
  else if (!strcmp(command, "M2R")) startPulse(2, false);
  else if (commandLength) { stopAll(); Serial.println(F("ERROR,UNKNOWN_COMMAND")); }
  commandLength = 0; overflow = false;
}

void setup() {
  const uint8_t pwmPins[] = {8, 9, 10, 11, 12};
  for (uint8_t p : pwmPins) { digitalWrite(p, LOW); pinMode(p, OUTPUT); analogWrite(p, 0); }
  for (uint8_t p = 22; p <= 29; ++p) { digitalWrite(p, LOW); pinMode(p, OUTPUT); }
  stopAll();
  for (uint8_t i = 0; i < 2; ++i) {
    pinMode(pins[i].a, INPUT_PULLUP); pinMode(pins[i].b, INPUT_PULLUP);
  }
  attachInterrupt(digitalPinToInterrupt(pins[0].a), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pins[1].a), encoder2ISR, CHANGE);
  Serial.begin(115200);
  Serial.println(F("READY,TWO_MOTOR_V2,M1=24/25/11,A19/B18,M2=22/23/10,A2/B35"));
  Serial.println(F("F/R: both; M1F/M1R/M2F/M2R: one; STOP; STATUS; RST. Newline required."));
}

void loop() {
  if (running && uint32_t(millis() - startedAt) >= PULSE_MS) {
    stopAll(); Serial.println(F("STOPPED,TIMEOUT")); reportEncoders();
    lastReport = millis();
  }
  // No telemetry writes while running: serial backpressure must not delay stop.
  if (!running && uint32_t(millis() - lastReport) >= REPORT_MS) {
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
