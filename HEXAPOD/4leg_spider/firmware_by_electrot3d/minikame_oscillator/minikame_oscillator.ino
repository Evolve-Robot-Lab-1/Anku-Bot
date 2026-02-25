/*
 * MiniKame Oscillator Test
 * Smooth sinusoidal movement with PCA9685
 *
 * Commands (Serial 9600):
 *   h - Home position
 *   w - Walk forward (4 steps)
 *   s - Walk backward (4 steps)
 *   a - Turn left
 *   d - Turn right
 *   r - Run forward
 *   1 - Dance
 *   2 - Up-Down
 *   3 - Front-Back
 *   4 - Moonwalk
 *   5 - Push up
 *   6 - Hello
 *   7 - Jump
 */

#include "MiniKame.h"

MiniKame robot;

void setup() {
  Serial.begin(9600);
  Serial.println(F(""));
  Serial.println(F("================================"));
  Serial.println(F("  MiniKame Oscillator Control"));
  Serial.println(F("================================"));
  Serial.println(F(""));

  robot.init();

  Serial.println(F("Robot initialized!"));
  Serial.println(F(""));
  Serial.println(F("Commands:"));
  Serial.println(F("  h = Home"));
  Serial.println(F("  w = Walk forward"));
  Serial.println(F("  s = Walk backward"));
  Serial.println(F("  a = Turn left"));
  Serial.println(F("  d = Turn right"));
  Serial.println(F("  r = Run"));
  Serial.println(F("  1 = Dance"));
  Serial.println(F("  2 = Up-Down"));
  Serial.println(F("  3 = Front-Back"));
  Serial.println(F("  4 = Moonwalk"));
  Serial.println(F("  5 = Push up"));
  Serial.println(F("  6 = Hello"));
  Serial.println(F("  7 = Jump"));
  Serial.println(F(""));
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'h':
      case 'H':
        Serial.println(F("Home..."));
        robot.home();
        break;

      case 'w':
      case 'W':
        Serial.println(F("Walk forward..."));
        robot.walk(1, 4, 550);
        break;

      case 's':
      case 'S':
        Serial.println(F("Walk backward..."));
        robot.walk(0, 4, 550);
        break;

      case 'a':
      case 'A':
        Serial.println(F("Turn left..."));
        robot.turnL(4, 550);
        break;

      case 'd':
      case 'D':
        Serial.println(F("Turn right..."));
        robot.turnR(4, 550);
        break;

      case 'r':
      case 'R':
        Serial.println(F("Run..."));
        robot.run(1, 4, 350);
        break;

      case '1':
        Serial.println(F("Dance..."));
        robot.dance(2, 2000);
        break;

      case '2':
        Serial.println(F("Up-Down..."));
        robot.upDown(4, 500);
        break;

      case '3':
        Serial.println(F("Front-Back..."));
        robot.frontBack(2, 2000);
        break;

      case '4':
        Serial.println(F("Moonwalk..."));
        robot.moonwalkL(2, 2000);
        break;

      case '5':
        Serial.println(F("Push up..."));
        robot.pushUp(2, 5000);
        break;

      case '6':
        Serial.println(F("Hello..."));
        robot.hello();
        robot.home();
        break;

      case '7':
        Serial.println(F("Jump..."));
        robot.jump();
        break;
    }
  }
}
