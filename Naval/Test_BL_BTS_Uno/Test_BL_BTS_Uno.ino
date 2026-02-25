/*
 * Test BL BTS7960 driver with Arduino Uno
 * RPWM=Pin 5, LPWM=Pin 6 (PWM capable on Uno)
 */

#define RPWM  5
#define LPWM  6

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
  Serial.println(">> BL FORWARD");
  analogWrite(RPWM, 180);
  analogWrite(LPWM, 0);
  delay(3000);

  // Stop
  Serial.println(">> STOP");
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
  delay(2000);

  // Backward
  Serial.println(">> BL BACKWARD");
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
