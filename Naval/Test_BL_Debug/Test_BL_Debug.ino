/*
 * BL Debug: Forward 3s, Stop 2s, Backward 3s, Stop 2s, repeat
 * ENA=Pin 7, IN1=Pin 42, IN2=Pin 43
 */

#define ENA  4
#define IN1  42
#define IN2  36

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  Serial.println(">> BL Forward/Backward sweep");
}

void loop() {
  // Forward
  Serial.println(">> FORWARD");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 180);
  delay(3000);

  // Stop
  Serial.println(">> STOP");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  delay(2000);

  // Backward
  Serial.println(">> BACKWARD");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 180);
  delay(3000);

  // Stop
  Serial.println(">> STOP");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  delay(2000);
}
