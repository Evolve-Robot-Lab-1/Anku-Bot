/*
 * Test Front Right Motor (Motor 2)
 * ENB=Pin 2, IN3=Pin 22, IN4=Pin 23
 */

#define ENB  2
#define IN3  22
#define IN4  23

void setup() {
  Serial.begin(115200);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(13, OUTPUT);

  for (int i = 0; i < 3; i++) {
    digitalWrite(13, HIGH); delay(300);
    digitalWrite(13, LOW);  delay(300);
  }

  Serial.println(">> Front Right FORWARD");
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 150);
  delay(3000);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 0);
  Serial.println(">> STOPPED");
}

void loop() {
}
