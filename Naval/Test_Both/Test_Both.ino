/*
 * Test BL + BR running together
 * BL: ENA=Pin 4, IN1=Pin 26, IN2=Pin 27 (L298N #2 Ch A)
 * BR: ENB=Pin 5, IN3=Pin 28, IN4=Pin 29 (L298N #2 Ch B)
 */

// BL
#define ENA  4
#define IN1  26
#define IN2  27

// BR
#define ENB  5
#define IN3  28
#define IN4  29

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // BL forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 150);

  // BR forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 150);

  Serial.println(">> BL + BR running");
}

void loop() {
}
