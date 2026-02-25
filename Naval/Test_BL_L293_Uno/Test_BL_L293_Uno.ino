/*
 * Test BL Motor with L293D on Arduino Uno
 * EN1=Pin 5 (PWM), IN1=Pin 6, IN2=Pin 7
 */

#define EN1  5
#define IN1  6
#define IN2  7

void setup() {
  Serial.begin(115200);
  pinMode(EN1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(13, OUTPUT);

  for (int i = 0; i < 3; i++) {
    digitalWrite(13, HIGH); delay(300);
    digitalWrite(13, LOW);  delay(300);
  }

  // Forward
  Serial.println(">> BL FORWARD");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(EN1, 180);
  delay(3000);

  // Stop
  Serial.println(">> STOP");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(EN1, 0);
  delay(2000);

  // Backward
  Serial.println(">> BL BACKWARD");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(EN1, 180);
  delay(3000);

  // Stop
  Serial.println(">> STOP");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(EN1, 0);
}

void loop() {
}
