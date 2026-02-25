/*
 * Test BL Motor with L293D on Arduino Mega
 * EN1=Pin 4 (PWM), IN1=Pin 7, IN2=Pin 9
 */

#define EN1  4
#define IN1  7
#define IN2  9

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
