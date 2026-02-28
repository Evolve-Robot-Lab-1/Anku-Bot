#include <Arduino.h>

// Arduino Mega: one driver channel, encoder A-edge decoding (x2).
const uint8_t ENCODER_A = 19;
const uint8_t ENCODER_B = 18;
volatile long encoderTicks = 0;
volatile unsigned long encoderEdges = 0;
unsigned long lastEncoderReport = 0;

void encoderISR() {
  encoderTicks += digitalRead(ENCODER_A) == digitalRead(ENCODER_B) ? 1 : -1;
  ++encoderEdges;
}

void reportEncoder() {
  noInterrupts();
  long ticks = encoderTicks;
  unsigned long edges = encoderEdges;
  interrupts();
  Serial.print(F("ENC,"));
  Serial.print(ticks);
  Serial.print(F(",EDGES,"));
  Serial.print(edges);
  Serial.print(F(",A,"));
  Serial.print(digitalRead(ENCODER_A));
  Serial.print(F(",B,"));
  Serial.println(digitalRead(ENCODER_B));
}
const uint8_t IN1 = 24;  // Driver IN3
const uint8_t IN2 = 25;  // Driver IN4
const uint8_t ENABLE_PWM = 11;  // Driver ENB
const uint8_t TEST_PWM = 200;
const unsigned long PULSE_MS = 1000;
bool running = false;
unsigned long startedAt = 0;
char command[16];
uint8_t commandLength = 0;
bool overflow = false;

void stopMotor() {
  analogWrite(ENABLE_PWM, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  running = false;
}

void executeCommand() {
  command[commandLength] = '\0';
  if (overflow) {
    stopMotor();
    Serial.println(F("ERROR,COMMAND_TOO_LONG"));
  } else if (!strcmp(command, "STOP") || !strcmp(command, "S")) {
    stopMotor();
    Serial.println(F("STOPPED"));
  } else if (!strcmp(command, "STATUS")) {
    Serial.println(running ? F("RUNNING") : F("STOPPED"));
    reportEncoder();
  } else if (!strcmp(command, "ENC")) {
    reportEncoder();
  } else if (!strcmp(command, "RST")) {
    if (running) Serial.println(F("ERROR,BUSY"));
    else {
      noInterrupts();
      encoderTicks = 0;
      encoderEdges = 0;
      interrupts();
      reportEncoder();
    }
  } else if (!strcmp(command, "F") || !strcmp(command, "R")) {
    if (running) {
      Serial.println(F("ERROR,BUSY"));
    } else {
      bool forward = command[0] == 'F';
      digitalWrite(IN1, forward ? HIGH : LOW);
      digitalWrite(IN2, forward ? LOW : HIGH);
      startedAt = millis();
      running = true;
      analogWrite(ENABLE_PWM, TEST_PWM);
      Serial.println(forward ? F("START,F,200,1000") : F("START,R,200,1000"));
    }
  } else if (commandLength) {
    stopMotor();
    Serial.println(F("ERROR,UNKNOWN_COMMAND"));
  }
  commandLength = 0;
  overflow = false;
}

void setup() {
  // Hold all wheel channels from the saved four-wheel map stopped.
  const uint8_t pwmPins[] = {9, 10, 11, 12};
  for (uint8_t pin : pwmPins) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
  }
  for (uint8_t pin = 22; pin <= 29; ++pin) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  stopMotor();
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, CHANGE);
  Serial.begin(115200);
  Serial.println(F("READY,SINGLE_MOTOR_V5,IN3=24,IN4=25,ENB=11,ENC_A=19,ENC_B=18"));
  Serial.println(F("F/R: PWM200 pulse 1s; STOP/S: stop; STATUS: state. Send newline."));
}

void loop() {
  if (running && millis() - startedAt >= PULSE_MS) {
    stopMotor();
    Serial.println(F("STOPPED,TIMEOUT"));
  }
  if (millis() - lastEncoderReport >= 100) {
    lastEncoderReport = millis();
    reportEncoder();
  }
  // Limit work per iteration so serial input cannot delay pulse timeout.
  for (uint8_t n = 0; n < 16 && Serial.available(); ++n) {
    char c = Serial.read();
    if (c == '\n') executeCommand();
    else if (c != '\r') {
      if (commandLength < sizeof(command) - 1) command[commandLength++] = c;
      else overflow = true;
    }
  }
}
