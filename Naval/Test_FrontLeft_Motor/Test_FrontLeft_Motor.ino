/*
 * AUTO TEST: Front Left Motor (Motor 1)
 * ENA=Pin 2, IN1=Pin 22, IN2=Pin 23
 */

#define ENA  2
#define IN1  22
#define IN2  23

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(13, OUTPUT);

  for (int i = 0; i < 3; i++) {
    digitalWrite(13, HIGH); delay(300);
    digitalWrite(13, LOW);  delay(300);
  }

  Serial.println(">> Motor 1 FORWARD");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 150);
  delay(3000);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  Serial.println(">> STOPPED");
}

void loop() {
}
