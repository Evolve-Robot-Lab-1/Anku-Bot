/*
 * Test BACK motors - BL and BR
 * BL: IN1=26, IN2=27, EN=10
 * BR: IN1=28, IN2=29, EN=9
 */

void setup() {
  Serial.begin(115200);
  Serial.println("BACK MOTOR TEST - BL and BR");

  // Back-Left
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(10, OUTPUT);

  // Back-Right
  pinMode(28, OUTPUT);
  pinMode(29, OUTPUT);
  pinMode(9, OUTPUT);

  // Both forward
  digitalWrite(26, HIGH); digitalWrite(27, LOW);  // BL forward
  digitalWrite(28, HIGH); digitalWrite(29, LOW);  // BR forward

  // Enable both
  digitalWrite(10, HIGH);
  digitalWrite(9, HIGH);

  Serial.println("Both back motors running FORWARD!");
}

void loop() {
  Serial.println("BL + BR running...");
  delay(1000);
}
