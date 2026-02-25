/*
 * Mecanum Wheel Control - Serial Only
 *
 * FL: ENA=8,  IN1=40, IN2=41 (L298N #1 Ch A)
 * FR: ENB=2,  IN3=22, IN4=23 (L298N #1 Ch B)
 * BL: ENA=4,  IN1=42, IN2=36 (L298N #2 Ch A)
 * BR: ENB=5,  IN3=28, IN4=29 (L298N #2 Ch B)
 *
 * Commands (Serial Monitor @ 115200):
 *   F = Forward
 *   B = Backward
 *   L = Strafe Left
 *   R = Strafe Right
 *   Q = Rotate Left
 *   E = Rotate Right
 *   S = Stop
 *   1 = Speed 120
 *   2 = Speed 180
 *   3 = Speed 230
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
#define BL_IN1  42
#define BL_IN2  36

// BR
#define BR_EN   5
#define BR_IN1  28
#define BR_IN2  29

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
  if (dir > 0) { digitalWrite(BR_IN1, HIGH); digitalWrite(BR_IN2, LOW); }
  else if (dir < 0) { digitalWrite(BR_IN1, LOW); digitalWrite(BR_IN2, HIGH); }
  else { digitalWrite(BR_IN1, LOW); digitalWrite(BR_IN2, LOW); }
  analogWrite(BR_EN, s);
}

void stopAll() {
  motorFL(0, 0); motorFR(0, 0); motorBL(0, 0); motorBR(0, 0);
}

void setup() {
  Serial.begin(115200);
  int pins[] = {FL_EN, FL_IN1, FL_IN2, FR_EN, FR_IN1, FR_IN2,
                BL_EN, BL_IN1, BL_IN2, BR_EN, BR_IN1, BR_IN2};
  for (int i = 0; i < 12; i++) pinMode(pins[i], OUTPUT);

  stopAll();
  Serial.println("=== Mecanum Control ===");
  Serial.println("F/B/L/R/Q/E/S  Speed:1/2/3");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
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
      case 'Q': case 'q':
        motorFL(-1, spd); motorFR(1, spd); motorBL(-1, spd); motorBR(1, spd);
        Serial.println(">> Rotate Left");
        break;
      case 'E': case 'e':
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
}
