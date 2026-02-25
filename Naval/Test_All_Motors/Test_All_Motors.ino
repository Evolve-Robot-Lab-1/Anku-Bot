/*
 * Test All 4 Motors - Combined
 * FL (L293D): ENA=8, IN1=40, IN2=41
 * FR (L293D): ENB=2, IN3=22, IN4=23
 * BL (L293D): EN1=4, IN1=7, IN2=9
 * BR (BTS7960): RPWM=10, LPWM=11
 */

// FL
#define FL_EN   8
#define FL_IN1  40
#define FL_IN2  41

// FR
#define FR_EN   2
#define FR_IN1  22
#define FR_IN2  23

// BL
#define BL_EN   4
#define BL_IN1  7
#define BL_IN2  9

// BR (BTS7960)
#define BR_RPWM 10
#define BR_LPWM 11

void stopAll() {
  analogWrite(FL_EN, 0);
  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, LOW);

  analogWrite(FR_EN, 0);
  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, LOW);

  analogWrite(BL_EN, 0);
  digitalWrite(BL_IN1, LOW);
  digitalWrite(BL_IN2, LOW);

  analogWrite(BR_RPWM, 0);
  analogWrite(BR_LPWM, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(FL_EN, OUTPUT);
  pinMode(FL_IN1, OUTPUT);
  pinMode(FL_IN2, OUTPUT);

  pinMode(FR_EN, OUTPUT);
  pinMode(FR_IN1, OUTPUT);
  pinMode(FR_IN2, OUTPUT);

  pinMode(BL_EN, OUTPUT);
  pinMode(BL_IN1, OUTPUT);
  pinMode(BL_IN2, OUTPUT);

  pinMode(BR_RPWM, OUTPUT);
  pinMode(BR_LPWM, OUTPUT);

  pinMode(13, OUTPUT);

  // LED blink 3 times
  for (int i = 0; i < 3; i++) {
    digitalWrite(13, HIGH); delay(300);
    digitalWrite(13, LOW);  delay(300);
  }

  // --- Test each motor individually ---

  // FL Forward
  Serial.println(">> FL FORWARD");
  digitalWrite(FL_IN1, HIGH);
  digitalWrite(FL_IN2, LOW);
  analogWrite(FL_EN, 180);
  delay(2000);
  stopAll();
  delay(1000);

  // FL Backward
  Serial.println(">> FL BACKWARD");
  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, HIGH);
  analogWrite(FL_EN, 180);
  delay(2000);
  stopAll();
  delay(1000);

  // FR Forward
  Serial.println(">> FR FORWARD");
  digitalWrite(FR_IN1, HIGH);
  digitalWrite(FR_IN2, LOW);
  analogWrite(FR_EN, 180);
  delay(2000);
  stopAll();
  delay(1000);

  // FR Backward
  Serial.println(">> FR BACKWARD");
  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, HIGH);
  analogWrite(FR_EN, 180);
  delay(2000);
  stopAll();
  delay(1000);

  // BL Forward
  Serial.println(">> BL FORWARD");
  digitalWrite(BL_IN1, HIGH);
  digitalWrite(BL_IN2, LOW);
  analogWrite(BL_EN, 180);
  delay(2000);
  stopAll();
  delay(1000);

  // BL Backward
  Serial.println(">> BL BACKWARD");
  digitalWrite(BL_IN1, LOW);
  digitalWrite(BL_IN2, HIGH);
  analogWrite(BL_EN, 180);
  delay(2000);
  stopAll();
  delay(1000);

  // BR Forward
  Serial.println(">> BR FORWARD");
  analogWrite(BR_RPWM, 180);
  analogWrite(BR_LPWM, 0);
  delay(2000);
  stopAll();
  delay(1000);

  // BR Backward
  Serial.println(">> BR BACKWARD");
  analogWrite(BR_RPWM, 0);
  analogWrite(BR_LPWM, 180);
  delay(2000);
  stopAll();
  delay(1000);

  // --- All motors forward together ---
  Serial.println(">> ALL FORWARD");
  digitalWrite(FL_IN1, HIGH);
  digitalWrite(FL_IN2, LOW);
  analogWrite(FL_EN, 180);

  digitalWrite(FR_IN1, HIGH);
  digitalWrite(FR_IN2, LOW);
  analogWrite(FR_EN, 180);

  digitalWrite(BL_IN1, HIGH);
  digitalWrite(BL_IN2, LOW);
  analogWrite(BL_EN, 180);

  analogWrite(BR_RPWM, 180);
  analogWrite(BR_LPWM, 0);
  delay(3000);
  stopAll();
  delay(1000);

  // --- All motors backward together ---
  Serial.println(">> ALL BACKWARD");
  digitalWrite(FL_IN1, LOW);
  digitalWrite(FL_IN2, HIGH);
  analogWrite(FL_EN, 180);

  digitalWrite(FR_IN1, LOW);
  digitalWrite(FR_IN2, HIGH);
  analogWrite(FR_EN, 180);

  digitalWrite(BL_IN1, LOW);
  digitalWrite(BL_IN2, HIGH);
  analogWrite(BL_EN, 180);

  analogWrite(BR_RPWM, 0);
  analogWrite(BR_LPWM, 180);
  delay(3000);
  stopAll();

  Serial.println(">> TEST COMPLETE");
}

void loop() {
}
