/*
 * Test Back Left Motor (Motor 3)
 * ENA=Pin 4, IN1=Pin 26, IN2=Pin 27
 */

#define ENA  4
#define IN1  26
#define IN2  27

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 150);
  Serial.println(">> Back Left running continuous");
}

void loop() {
}
