/*
 * DEBUG: Raw encoder pin read while motor spins
 */

#define ENB  2
#define IN3  22
#define IN4  23

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(19, INPUT_PULLUP);
  pinMode(31, INPUT_PULLUP);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 150);
  Serial.println(">> Motor ON - reading raw pins");
}

void loop() {
  Serial.print("ChA(19)=");
  Serial.print(digitalRead(19));
  Serial.print("  ChB(31)=");
  Serial.println(digitalRead(31));
  delay(50);
}
