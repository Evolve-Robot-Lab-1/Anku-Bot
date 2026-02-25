/*
 * Test Back Right Motor (Motor 4)
 * ENB=Pin 5, IN3=Pin 28, IN4=Pin 29
 */

#define ENB  10
#define IN3  44
#define IN4  45

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

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 180);
  Serial.println(">> Back Right running");
}

void loop() {
}
