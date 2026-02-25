/*
 * Mecanum Wheel Control - Serial + Bluetooth (HC-05)
 *
 * FL (L293D): EN=8,  IN1=40, IN2=41
 * FR (L293D): EN=2,  IN1=22, IN2=23
 * BL (L293D): EN=4,  IN1=7,  IN2=9
 * BR (BTS7960): RPWM=10, LPWM=11
 *
 * HC-05: TXD -> Pin 17 (RX2), RXD -> Pin 16 (TX2)
 *
 * Commands:
 *   F = Forward       B = Backward
 *   L = Strafe Left   R = Strafe Right
 *   W = Rotate Left   U = Rotate Right
 *   S = Stop
 *   1 = Speed 120     2 = Speed 180     3 = Speed 230
 */

// FL
#define FL_EN   8
#define FL_IN1  40
#define FL_IN2  41

// FR
#define FR_EN   2
#define FR_IN1  22
#define FR_IN2  23

// BL
#define BL_EN   4
#define BL_IN1  7
#define BL_IN2  9

// BR (BTS7960)
#define BR_RPWM 10
#define BR_LPWM 11

int spd = 180;

void motorFL(int dir, int s) {
  if (dir > 0) { digitalWrite(FL_IN1, HIGH); digitalWrite(FL_IN2, LOW); }
  else if (dir < 0) { digitalWrite(FL_IN1, LOW); digitalWrite(FL_IN2, HIGH); }
  else { digitalWrite(FL_IN1, LOW); digitalWrite(FL_IN2, LOW); }
  analogWrite(FL_EN, s);
}

void motorFR(int dir, int s) {
  if (dir > 0) { digitalWrite(FR_IN1, HIGH); digitalWrite(FR_IN2, LOW); }
  else if (dir < 0) { digitalWrite(FR_IN1, LOW); digitalWrite(FR_IN2, HIGH); }
  else { digitalWrite(FR_IN1, LOW); digitalWrite(FR_IN2, LOW); }
  analogWrite(FR_EN, s);
}

void motorBL(int dir, int s) {
  if (dir > 0) { digitalWrite(BL_IN1, HIGH); digitalWrite(BL_IN2, LOW); }
  else if (dir < 0) { digitalWrite(BL_IN1, LOW); digitalWrite(BL_IN2, HIGH); }
  else { digitalWrite(BL_IN1, LOW); digitalWrite(BL_IN2, LOW); }
  analogWrite(BL_EN, s);
}

void motorBR(int dir, int s) {
  if (dir > 0) { analogWrite(BR_RPWM, s); analogWrite(BR_LPWM, 0); }
  else if (dir < 0) { analogWrite(BR_RPWM, 0); analogWrite(BR_LPWM, s); }
  else { analogWrite(BR_RPWM, 0); analogWrite(BR_LPWM, 0); }
}

void stopAll() {
  motorFL(0, 0); motorFR(0, 0); motorBL(0, 0); motorBR(0, 0);
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'F': case 'f':
      motorFL(1, spd); motorFR(1, spd); motorBL(1, spd); motorBR(1, spd);
      Serial.println(">> Forward");
      break;
    case 'B': case 'b':
      motorFL(-1, spd); motorFR(-1, spd); motorBL(-1, spd); motorBR(-1, spd);
      Serial.println(">> Backward");
      break;
    case 'L': case 'l':
      motorFL(-1, spd); motorFR(1, spd); motorBL(1, spd); motorBR(-1, spd);
      Serial.println(">> Strafe Left");
      break;
    case 'R': case 'r':
      motorFL(1, spd); motorFR(-1, spd); motorBL(-1, spd); motorBR(1, spd);
      Serial.println(">> Strafe Right");
      break;
    case 'W': case 'w':
      motorFL(-1, spd); motorFR(1, spd); motorBL(-1, spd); motorBR(1, spd);
      Serial.println(">> Rotate Left");
      break;
    case 'U': case 'u':
      motorFL(1, spd); motorFR(-1, spd); motorBL(1, spd); motorBR(-1, spd);
      Serial.println(">> Rotate Right");
      break;
    case 'S': case 's':
      stopAll();
      Serial.println(">> Stop");
      break;
    case '1': spd = 120; Serial.println(">> Speed 120"); break;
    case '2': spd = 180; Serial.println(">> Speed 180"); break;
    case '3': spd = 230; Serial.println(">> Speed 230"); break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);

  pinMode(FL_EN, OUTPUT);
  pinMode(FL_IN1, OUTPUT);
  pinMode(FL_IN2, OUTPUT);

  pinMode(FR_EN, OUTPUT);
  pinMode(FR_IN1, OUTPUT);
  pinMode(FR_IN2, OUTPUT);

  pinMode(BL_EN, OUTPUT);
  pinMode(BL_IN1, OUTPUT);
  pinMode(BL_IN2, OUTPUT);

  pinMode(BR_RPWM, OUTPUT);
  pinMode(BR_LPWM, OUTPUT);

  stopAll();
  Serial.println("=== Mecanum Control + Bluetooth ===");
  Serial.println("F/B/L/R/W/U/S  Speed:1/2/3");
}

void loop() {
  if (Serial2.available()) {
    char cmd = Serial2.read();
    handleCommand(cmd);
  }
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
}
