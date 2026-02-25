/*
 * Test FL: Continuous forward
 * ENA=Pin 8, IN1=Pin 40, IN2=Pin 41
 */

#define ENA  8
#define IN1  40
#define IN2  41

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 180);
  Serial.println(">> FL running continuous");
}

void loop() {
}
