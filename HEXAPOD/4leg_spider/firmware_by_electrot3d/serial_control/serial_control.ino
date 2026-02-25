/*
 * Simple Serial Control - All in one file
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 125
#define SERVOMAX 575

// Channels
#define FR_HIP 0
#define FL_HIP 1
#define FR_LEG 2
#define FL_LEG 3
#define BR_HIP 4
#define BL_HIP 5
#define BR_LEG 6
#define BL_LEG 7

// Home positions
int homeHip[4] = {70, 110, 110, 70};
int homeLeg = 90;

void setAngle(int ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(ch, 0, pulse);
}

void home() {
  setAngle(FR_HIP, homeHip[0]);
  setAngle(FL_HIP, homeHip[1]);
  setAngle(BR_HIP, homeHip[2]);
  setAngle(BL_HIP, homeHip[3]);
  setAngle(FR_LEG, homeLeg);
  setAngle(FL_LEG, homeLeg);
  setAngle(BR_LEG, homeLeg);
  setAngle(BL_LEG, homeLeg);
}

void walkForward() {
  int hipSwing = 25;
  int legLift = 35;

  for (int s = 0; s < 8; s++) {
    setAngle(FR_LEG, homeLeg - legLift);
    setAngle(BL_LEG, homeLeg + legLift);
    delay(100);

    setAngle(FR_HIP, homeHip[0] + hipSwing);
    setAngle(BL_HIP, homeHip[3] - hipSwing);
    setAngle(FL_HIP, homeHip[1] + hipSwing);
    setAngle(BR_HIP, homeHip[2] - hipSwing);
    delay(150);

    setAngle(FR_LEG, homeLeg);
    setAngle(BL_LEG, homeLeg);
    delay(100);

    setAngle(FL_LEG, homeLeg + legLift);
    setAngle(BR_LEG, homeLeg - legLift);
    delay(100);

    setAngle(FL_HIP, homeHip[1] - hipSwing);
    setAngle(BR_HIP, homeHip[2] + hipSwing);
    setAngle(FR_HIP, homeHip[0] - hipSwing);
    setAngle(BL_HIP, homeHip[3] + hipSwing);
    delay(150);

    setAngle(FL_LEG, homeLeg);
    setAngle(BR_LEG, homeLeg);
    delay(100);
  }
  home();
}

void dance() {
  int legSwing = 35;
  for (int s = 0; s < 4; s++) {
    setAngle(FR_LEG, homeLeg - legSwing);
    setAngle(BR_LEG, homeLeg - legSwing);
    setAngle(FL_LEG, homeLeg + legSwing);
    setAngle(BL_LEG, homeLeg + legSwing);
    delay(300);

    setAngle(FR_LEG, homeLeg + legSwing);
    setAngle(BR_LEG, homeLeg + legSwing);
    setAngle(FL_LEG, homeLeg - legSwing);
    setAngle(BL_LEG, homeLeg - legSwing);
    delay(300);
  }
  home();
}

void hello() {
  setAngle(FR_LEG, homeLeg - 45);
  setAngle(FL_LEG, homeLeg + 45);
  setAngle(BR_LEG, homeLeg + 25);
  setAngle(BL_LEG, homeLeg - 25);
  delay(500);

  setAngle(FR_LEG, homeLeg + 50);
  delay(300);

  for (int i = 0; i < 3; i++) {
    setAngle(FR_HIP, homeHip[0] - 25);
    delay(250);
    setAngle(FR_HIP, homeHip[0] + 25);
    delay(250);
  }
  delay(300);
  home();
}

void setup() {
  Serial.begin(9600);

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  delay(10);

  home();

  Serial.println("Ready! Commands: w=walk, 1=dance, 6=hello, h=home");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    Serial.print("Got: ");
    Serial.println(cmd);

    if (cmd == 'w' || cmd == 'W') {
      Serial.println("Walking...");
      walkForward();
      Serial.println("Done");
    }
    else if (cmd == '1') {
      Serial.println("Dancing...");
      dance();
      Serial.println("Done");
    }
    else if (cmd == '6') {
      Serial.println("Hello...");
      hello();
      Serial.println("Done");
    }
    else if (cmd == 'h' || cmd == 'H') {
      Serial.println("Home");
      home();
    }
  }
}
