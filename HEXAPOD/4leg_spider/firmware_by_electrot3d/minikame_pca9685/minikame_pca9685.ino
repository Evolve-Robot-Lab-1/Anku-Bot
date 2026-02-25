/*
 * MiniKame Quadruped Robot - PCA9685 Version
 * Full robot firmware with serial control
 * SLOWER movements
 */

#include "MiniKame_PCA.h"

MiniKame_PCA robot;

void printCommands() {
  Serial.println("\n=== MiniKame PCA9685 ===");
  Serial.println("w - Walk forward");
  Serial.println("s - Walk backward");
  Serial.println("a - Turn left");
  Serial.println("d - Turn right");
  Serial.println("h - Home position");
  Serial.println("1 - Dance");
  Serial.println("2 - Moonwalk");
  Serial.println("3 - Push up");
  Serial.println("6 - Hello wave");
  Serial.println("7 - Jump");
}

void setup() {
  Serial.begin(9600);
  Serial.println("\nInitializing robot...");
  robot.init();
  Serial.println("Robot ready!");
  printCommands();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'w':
      case 'W':
        Serial.println("Walking forward...");
        robot.walk(8, 550, 1);  // 8 steps
        Serial.println("Done");
        break;

      case 's':
      case 'S':
        Serial.println("Walking backward...");
        robot.walk(8, 550, 0);  // 8 steps
        Serial.println("Done");
        break;

      case 'a':
      case 'A':
        Serial.println("Turning left...");
        robot.turnLeft(6, 550);  // 6 steps
        Serial.println("Done");
        break;

      case 'd':
      case 'D':
        Serial.println("Turning right...");
        robot.turnRight(6, 550);  // 6 steps
        Serial.println("Done");
        break;

      case 'h':
      case 'H':
        Serial.println("Home position");
        robot.home();
        break;

      case '1':
        Serial.println("Dancing...");
        robot.dance(4, 2000);  // 4 cycles
        Serial.println("Done");
        break;

      case '2':
        Serial.println("Moonwalking...");
        robot.moonwalk(4, 2000);  // 4 cycles
        Serial.println("Done");
        break;

      case '3':
        Serial.println("Push ups...");
        robot.pushUp(4, 2000);  // 4 push ups
        Serial.println("Done");
        break;

      case '6':
        Serial.println("Hello!");
        robot.hello();
        Serial.println("Done");
        break;

      case '7':
        Serial.println("Jumping!");
        robot.jump();
        Serial.println("Done");
        break;

      case '?':
        printCommands();
        break;
    }
  }
}
