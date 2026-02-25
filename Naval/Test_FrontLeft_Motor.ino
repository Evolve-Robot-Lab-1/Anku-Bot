/*
 * ============================================================
 * TEST: Front Left Motor (Motor 1) Only
 * ============================================================
 *
 * WIRING:
 *   L298N #1:
 *     ENA  -> Mega Pin 2   (PWM) - REMOVE JUMPER CAP!
 *     IN1  -> Mega Pin 22
 *     IN2  -> Mega Pin 23
 *     OUT1 -> Motor wire 1
 *     OUT2 -> Motor wire 2
 *     12V  -> 12V supply
 *     GND  -> Common GND
 *
 *   Encoder (Motor 1):
 *     VCC  -> 5V
 *     GND  -> GND
 *     ChA  -> Mega Pin 18
 *     ChB  -> Mega Pin 30
 *
 * COMMANDS (Serial Monitor @ 115200 baud):
 *   F = Forward
 *   B = Backward
 *   S = Stop
 *   1 = Slow (100)
 *   2 = Medium (150)
 *   3 = Fast (200)
 *   P = Print encoder count
 *   E = Reset encoder
 * ============================================================
 */

#define ENA  2
#define IN1  22
#define IN2  23

#define ENC_A 18
#define ENC_B 30

volatile long encoderCount = 0;
int speed = 150;

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, RISING);

  // Start stopped
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);

  Serial.println("================================");
  Serial.println("  Front Left Motor Test");
  Serial.println("================================");
  Serial.println("F=Forward  B=Backward  S=Stop");
  Serial.println("1=Slow  2=Medium  3=Fast");
  Serial.println("P=Print encoder  E=Reset encoder");
  Serial.println("================================");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'F': case 'f':
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, speed);
        Serial.print(">> Forward @ speed ");
        Serial.println(speed);
        break;
      case 'B': case 'b':
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        analogWrite(ENA, speed);
        Serial.print(">> Backward @ speed ");
        Serial.println(speed);
        break;
      case 'S': case 's':
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 0);
        Serial.println(">> Stopped");
        break;
      case '1':
        speed = 100;
        Serial.println(">> Speed: Slow (100)");
        break;
      case '2':
        speed = 150;
        Serial.println(">> Speed: Medium (150)");
        break;
      case '3':
        speed = 200;
        Serial.println(">> Speed: Fast (200)");
        break;
      case 'P': case 'p':
        Serial.print(">> Encoder: ");
        Serial.println(encoderCount);
        break;
      case 'E': case 'e':
        encoderCount = 0;
        Serial.println(">> Encoder reset to 0");
        break;
    }
  }
}

void encoderISR() {
  if (digitalRead(ENC_B) == LOW) encoderCount++;
  else encoderCount--;
}
