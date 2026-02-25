/*
 * Test FL + FR running together with LED
 * FL: ENA=Pin 8, IN1=Pin 40, IN2=Pin 41 (L298N #1 Ch A)
 * FR: ENB=Pin 2, IN3=Pin 22, IN4=Pin 23 (L298N #1 Ch B)
 */

// FL
#define ENA  8
#define IN1  40
#define IN2  41

// FR
#define ENB  2
#define IN3  22
#define IN4  23

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(13, OUTPUT);

  for (int i = 0; i < 3; i++) {
    digitalWrite(13, HIGH); delay(300);
    digitalWrite(13, LOW);  delay(300);
  }

  // FL forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 180);

  // FR forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 180);

  Serial.println(">> FL + FR running");
}

void loop() {
}
