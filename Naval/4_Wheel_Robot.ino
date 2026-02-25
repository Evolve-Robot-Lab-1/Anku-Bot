/*
 * ============================================================
 * 4-Wheel Robot with Arduino Mega
 * 4x PG36 Motors + 4x Encoders + HC-05 Bluetooth
 * ============================================================
 * 
 * COMPONENTS:
 *   - Arduino Mega 2560
 *   - 2x L298N Motor Drivers
 *   - 4x PG36555126000-19.2KE7 Motors (12V) with Encoders
 *   - HC-05 Bluetooth Module
 *   - 12V Power Supply (min 5A)
 * 
 * ============================================================
 * PIN CONNECTIONS:
 * ============================================================
 * 
 * L298N #1 (Motor 1 & Motor 2):
 *   ENA  -> Pin 2  (PWM)
 *   IN1  -> Pin 22
 *   IN2  -> Pin 23
 *   IN3  -> Pin 24
 *   IN4  -> Pin 25
 *   ENB  -> Pin 3  (PWM)
 *   OUT1/OUT2 -> Motor 1
 *   OUT3/OUT4 -> Motor 2
 * 
 * L298N #2 (Motor 3 & Motor 4):
 *   ENA  -> Pin 4  (PWM)
 *   IN1  -> Pin 26
 *   IN2  -> Pin 27
 *   IN3  -> Pin 28
 *   IN4  -> Pin 29
 *   ENB  -> Pin 5  (PWM)
 *   OUT1/OUT2 -> Motor 3
 *   OUT3/OUT4 -> Motor 4
 * 
 * Encoders:
 *   Motor 1: VCC->5V, GND->GND, ChA->Pin 18, ChB->Pin 30
 *   Motor 2: VCC->5V, GND->GND, ChA->Pin 19, ChB->Pin 31
 *   Motor 3: VCC->5V, GND->GND, ChA->Pin 20, ChB->Pin 32
 *   Motor 4: VCC->5V, GND->GND, ChA->Pin 21, ChB->Pin 33
 * 
 * HC-05 Bluetooth:
 *   VCC -> 5V
 *   GND -> GND
 *   TXD -> Pin 15 (RX3)
 *   RXD -> Pin 14 (TX3)
 * 
 * Power Supply:
 *   12V (+) -> L298N #1 (12V), L298N #2 (12V), Mega VIN
 *   12V (-) -> L298N #1 GND, L298N #2 GND, Mega GND
 * 
 * IMPORTANT:
 *   - Remove ENA and ENB jumper caps on BOTH L298N boards
 *   - All GNDs must be connected together
 *   - Use breadboard to share 5V for encoder VCCs
 * 
 * ============================================================
 * COMMANDS (Serial Monitor or Bluetooth):
 * ============================================================
 *   F = Forward
 *   B = Backward
 *   L = Turn Left
 *   R = Turn Right
 *   S = Stop
 *   1 = Slow speed
 *   2 = Medium speed
 *   3 = Fast speed
 *   A = All motors forward (test)
 *   E = Reset encoders
 * 
 * ============================================================
 */

// ==================== L298N #1 - Motor 1 & 2 ====================
#define ENA_1  2
#define IN1_1  22
#define IN2_1  23
#define IN3_1  24
#define IN4_1  25
#define ENB_1  3

// ==================== L298N #2 - Motor 3 & 4 ====================
#define ENA_2  4
#define IN1_2  26
#define IN2_2  27
#define IN3_2  28
#define IN4_2  29
#define ENB_2  5

// ==================== ENCODERS ====================
#define ENC1_A 18
#define ENC1_B 30
#define ENC2_A 19
#define ENC2_B 31
#define ENC3_A 20
#define ENC3_B 32
#define ENC4_A 21
#define ENC4_B 33

// ==================== VARIABLES ====================
volatile long enc1 = 0;
volatile long enc2 = 0;
volatile long enc3 = 0;
volatile long enc4 = 0;

int speed = 150;

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);   // Debug via USB
  Serial3.begin(9600);    // HC-05 Bluetooth

  // L298N #1 pins
  pinMode(ENA_1, OUTPUT);
  pinMode(IN1_1, OUTPUT);
  pinMode(IN2_1, OUTPUT);
  pinMode(IN3_1, OUTPUT);
  pinMode(IN4_1, OUTPUT);
  pinMode(ENB_1, OUTPUT);

  // L298N #2 pins
  pinMode(ENA_2, OUTPUT);
  pinMode(IN1_2, OUTPUT);
  pinMode(IN2_2, OUTPUT);
  pinMode(IN3_2, OUTPUT);
  pinMode(IN4_2, OUTPUT);
  pinMode(ENB_2, OUTPUT);

  // Encoder pins
  pinMode(ENC1_A, INPUT_PULLUP);
  pinMode(ENC1_B, INPUT_PULLUP);
  pinMode(ENC2_A, INPUT_PULLUP);
  pinMode(ENC2_B, INPUT_PULLUP);
  pinMode(ENC3_A, INPUT_PULLUP);
  pinMode(ENC3_B, INPUT_PULLUP);
  pinMode(ENC4_A, INPUT_PULLUP);
  pinMode(ENC4_B, INPUT_PULLUP);

  // Attach encoder interrupts
  attachInterrupt(digitalPinToInterrupt(ENC1_A), enc1ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC2_A), enc2ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC3_A), enc3ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC4_A), enc4ISR, RISING);

  stopAll();

  Serial.println("============================================");
  Serial.println("  4-Wheel Robot - Arduino Mega");
  Serial.println("============================================");
  Serial.println("Commands: F/B/L/R/S, Speed: 1/2/3");
  Serial.println("A=All motors, E=Reset encoders");
  Serial.println("============================================");
}

// ==================== MAIN LOOP ====================
void loop() {
  // Bluetooth commands
  if (Serial3.available()) {
    char cmd = Serial3.read();
    handleCommand(cmd);
  }

  // Serial Monitor commands (for testing without Bluetooth)
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
}

// ==================== COMMAND HANDLER ====================
void handleCommand(char cmd) {
  switch (cmd) {
    case 'F':
    case 'f':
      forward();
      Serial.println(">> Forward");
      break;
    case 'B':
    case 'b':
      backward();
      Serial.println(">> Backward");
      break;
    case 'L':
    case 'l':
      left();
      Serial.println(">> Left");
      break;
    case 'R':
    case 'r':
      right();
      Serial.println(">> Right");
      break;
    case 'S':
    case 's':
      stopAll();
      Serial.println(">> Stop");
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
    case 'A':
    case 'a':
      forward();
      Serial.println(">> All motors forward");
      break;
    case 'E':
    case 'e':
      enc1 = 0; enc2 = 0; enc3 = 0; enc4 = 0;
      Serial.println(">> Encoders reset");
      break;
    case 'P':
    case 'p':
      printEncoders();
      break;
  }
}

// ==================== MOVEMENT FUNCTIONS ====================
void forward() {
  motor1Fwd(speed); delay(50);
  motor2Fwd(speed); delay(50);
  motor3Fwd(speed); delay(50);
  motor4Fwd(speed);
}

void backward() {
  motor1Rev(speed); delay(50);
  motor2Rev(speed); delay(50);
  motor3Rev(speed); delay(50);
  motor4Rev(speed);
}

void left() {
  motor1Rev(speed); delay(50);
  motor2Rev(speed); delay(50);
  motor3Fwd(speed); delay(50);
  motor4Fwd(speed);
}

void right() {
  motor1Fwd(speed); delay(50);
  motor2Fwd(speed); delay(50);
  motor3Rev(speed); delay(50);
  motor4Rev(speed);
}

// ==================== MOTOR CONTROL ====================
void motor1Fwd(int s) { digitalWrite(IN1_1, HIGH); digitalWrite(IN2_1, LOW); analogWrite(ENA_1, s); }
void motor1Rev(int s) { digitalWrite(IN1_1, LOW); digitalWrite(IN2_1, HIGH); analogWrite(ENA_1, s); }
void motor2Fwd(int s) { digitalWrite(IN3_1, HIGH); digitalWrite(IN4_1, LOW); analogWrite(ENB_1, s); }
void motor2Rev(int s) { digitalWrite(IN3_1, LOW); digitalWrite(IN4_1, HIGH); analogWrite(ENB_1, s); }
void motor3Fwd(int s) { digitalWrite(IN1_2, HIGH); digitalWrite(IN2_2, LOW); analogWrite(ENA_2, s); }
void motor3Rev(int s) { digitalWrite(IN1_2, LOW); digitalWrite(IN2_2, HIGH); analogWrite(ENA_2, s); }
void motor4Fwd(int s) { digitalWrite(IN3_2, HIGH); digitalWrite(IN4_2, LOW); analogWrite(ENB_2, s); }
void motor4Rev(int s) { digitalWrite(IN3_2, LOW); digitalWrite(IN4_2, HIGH); analogWrite(ENB_2, s); }

void stopAll() {
  digitalWrite(IN1_1, LOW); digitalWrite(IN2_1, LOW); analogWrite(ENA_1, 0);
  digitalWrite(IN3_1, LOW); digitalWrite(IN4_1, LOW); analogWrite(ENB_1, 0);
  digitalWrite(IN1_2, LOW); digitalWrite(IN2_2, LOW); analogWrite(ENA_2, 0);
  digitalWrite(IN3_2, LOW); digitalWrite(IN4_2, LOW); analogWrite(ENB_2, 0);
}

// ==================== ENCODER ISRs ====================
void enc1ISR() { if (digitalRead(ENC1_B) == LOW) enc1++; else enc1--; }
void enc2ISR() { if (digitalRead(ENC2_B) == LOW) enc2++; else enc2--; }
void enc3ISR() { if (digitalRead(ENC3_B) == LOW) enc3++; else enc3--; }
void enc4ISR() { if (digitalRead(ENC4_B) == LOW) enc4++; else enc4--; }

// ==================== PRINT ENCODERS ====================
void printEncoders() {
  Serial.print("E1: "); Serial.print(enc1);
  Serial.print(" | E2: "); Serial.print(enc2);
  Serial.print(" | E3: "); Serial.print(enc3);
  Serial.print(" | E4: "); Serial.println(enc4);
}
