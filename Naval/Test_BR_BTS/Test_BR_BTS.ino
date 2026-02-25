/*
 * Test BR Motor with BTS7960
 * RPWM=Pin 10, LPWM=Pin 11
 */

#define RPWM  10
#define LPWM  11

void setup() {
  Serial.begin(115200);
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(13, OUTPUT);

  for (int i = 0; i < 3; i++) {
    digitalWrite(13, HIGH); delay(300);
    digitalWrite(13, LOW);  delay(300);
  }

  // Forward
  Serial.println(">> BR FORWARD");
  analogWrite(RPWM, 180);
  analogWrite(LPWM, 0);
  delay(3000);

  // Stop
  Serial.println(">> STOP");
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
  delay(2000);

  // Backward
  Serial.println(">> BR BACKWARD");
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 180);
  delay(3000);

  // Stop
  Serial.println(">> STOP");
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}

void loop() {
}
