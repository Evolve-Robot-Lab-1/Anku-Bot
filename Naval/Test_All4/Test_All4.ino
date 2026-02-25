/*
 * All 4 Motors Forward
 * FL: ENA=8,  IN1=40, IN2=41 (L298N #1 Ch A)
 * FR: ENB=2,  IN3=22, IN4=23 (L298N #1 Ch B)
 * BL: ENA=4,  IN1=26, IN2=27 (L298N #2 Ch A)
 * BR: ENB=5,  IN3=28, IN4=29 (L298N #2 Ch B)
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
#define BL_IN1  26
#define BL_IN2  27

// BR
#define BR_EN   5
#define BR_IN1  28
#define BR_IN2  29

void setup() {
  Serial.begin(115200);
  int pins[] = {FL_EN, FL_IN1, FL_IN2, FR_EN, FR_IN1, FR_IN2,
                BL_EN, BL_IN1, BL_IN2, BR_EN, BR_IN1, BR_IN2};
  for (int i = 0; i < 12; i++) pinMode(pins[i], OUTPUT);

  // FL forward
  digitalWrite(FL_IN1, HIGH); digitalWrite(FL_IN2, LOW); analogWrite(FL_EN, 180);
  // FR forward
  digitalWrite(FR_IN1, HIGH); digitalWrite(FR_IN2, LOW); analogWrite(FR_EN, 180);
  // BL forward
  digitalWrite(BL_IN1, HIGH); digitalWrite(BL_IN2, LOW); analogWrite(BL_EN, 180);
  // BR forward
  digitalWrite(BR_IN1, HIGH); digitalWrite(BR_IN2, LOW); analogWrite(BR_EN, 180);

  Serial.println(">> All 4 motors forward");
}

void loop() {
}
