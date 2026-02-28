#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Solenoid on CH6, pump on CH7; OE wired to Uno D8.
// No output at startup. Commands 1..3 test CH6 at 1000/1500/2000 us;
// commands 4..6 test CH7 at the same values. S disables PWM outputs.
// G tests suction/release using the observed responses. Valve airflow
// direction must be confirmed physically; a click alone does not establish it.
Adafruit_PWMServoDriver pca(0x41);
uint8_t channel = 7;
const uint8_t OE_PIN = 8;
const uint16_t PULSES[] = {1000, 1500, 2000};
bool found = false;
bool testing = false;
bool combined = false;
bool servoTesting = false;
bool bothServos = false;
uint8_t servoChannel = 0;
int servoAngle = 0;
int servoDirection = 1;
unsigned long servoStepAt = 0;
uint8_t step = 0;
uint8_t phase = 0;
const uint16_t COMBINED_PULSES[][2] = {
  {1500, 2000},  // Valve neutral, pump running (confirmed physically).
  {2000, 1500},  // Valve actuated, pump neutral.
  {1500, 1500}   // Both neutral before disabling PWM.
};
const uint16_t PHASE_MS[] = {1000, 500, 250};
unsigned long stepStarted = 0;

void stopTest() {
  digitalWrite(OE_PIN, HIGH);
  testing = false;
  combined = false;
  servoTesting = false;
  bothServos = false;
  if (found) {
    for (uint8_t ch = 0; ch < 16; ++ch) pca.setPWM(ch, 0, 4096);
  }
  Serial.println(F("OUTPUT DISABLED"));
}

void applyStep() {
  uint16_t us = PULSES[step];
  pca.setPWM(channel, 0, (uint32_t)us * 4096UL / 20000UL);
  digitalWrite(OE_PIN, LOW);
  stepStarted = millis();
  Serial.print(F("CH"));
  Serial.print(channel);
  Serial.print(F(" pulse us="));
  Serial.println(us);
}

void applyCombination() {
  pca.setPWM(6, 0, (uint32_t)COMBINED_PULSES[phase][0] * 4096UL / 20000UL);
  pca.setPWM(7, 0, (uint32_t)COMBINED_PULSES[phase][1] * 4096UL / 20000UL);
  digitalWrite(OE_PIN, LOW);
  stepStarted = millis();
  if (phase == 0) Serial.println(F("GRIP TEST: valve=1500, pump=2000, 1 second"));
  else if (phase == 1) Serial.println(F("RELEASE TEST: valve=2000, pump=1500, 0.5 seconds"));
  else Serial.println(F("BOTH NEUTRAL: 1500 us"));
}

void setup() {
  digitalWrite(OE_PIN, HIGH);
  pinMode(OE_PIN, OUTPUT);
  Serial.begin(115200);
  Wire.begin();
  Wire.setWireTimeout(25000UL, true);
  Wire.beginTransmission(0x41);
  found = Wire.endTransmission() == 0;
  if (!found) {
    Serial.println(F("ERROR: PCA missing"));
    return;
  }
  pca.begin();
  pca.setPWMFreq(50);
  for (uint8_t ch = 0; ch < 16; ++ch) pca.setPWM(ch, 0, 4096);
  Serial.println(F("READY: 1/2/3=CH6, 4/5/6=CH7; 1000/1500/2000 us. S=disable."));
  Serial.println(F("G=one suction/release test. No output at startup."));
  Serial.println(F("A=CH0, B=CH1, C=both: one nominal 0->60->0 sweep. S=stop."));
}

void loop() {
  if (!found) return;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'S' || c == 's') stopTest();
    else if ((c == 'A' || c == 'B' || c == 'C') && !testing) {
      stopTest();
      bothServos = c == 'C';
      servoChannel = c == 'A' ? 0 : 1;
      servoAngle = 0;
      servoDirection = 1;
      servoTesting = true;
      testing = true;
      servoStepAt = millis() - 25;
      if (bothServos) Serial.println(F("SERVO START CH0+CH1"));
      else {
        Serial.print(F("SERVO START CH"));
        Serial.println(servoChannel);
      }
    }
    else if ((c == 'G' || c == 'g') && !testing) {
      combined = true;
      testing = true;
      phase = 0;
      applyCombination();
    }
    else if (c >= '1' && c <= '6' && !testing) {
      channel = c <= '3' ? 6 : 7;
      step = (c - '1') % 3;
      testing = true;
      applyStep();
    }
  }
  if (servoTesting) {
    if (millis() - servoStepAt < 25) return;
    servoStepAt = millis();
    const uint16_t us = map(servoAngle, 0, 180, 1000, 2000);
    pca.setPWM(servoChannel, 0, (uint32_t)us * 4096UL / 20000UL);
    if (bothServos) pca.setPWM(0, 0, (uint32_t)us * 4096UL / 20000UL);
    digitalWrite(OE_PIN, LOW);
    if (servoAngle == 60) servoDirection = -1;
    else if (servoAngle == 0 && servoDirection == -1) {
      Serial.println(F("SERVO CYCLE COMPLETE"));
      stopTest();
      return;
    }
    servoAngle += servoDirection;
    return;
  }
  if (testing && millis() - stepStarted >= (combined ? PHASE_MS[phase] : 2000)) {
    if (combined && ++phase < sizeof(PHASE_MS) / sizeof(PHASE_MS[0])) applyCombination();
    else stopTest();
  }
}
