/*
 * MiniKame PCA9685 Version
 * Quadruped robot control library for PCA9685 servo driver
 */

#ifndef MINIKAME_PCA_H
#define MINIKAME_PCA_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Servo channels on PCA9685
#define FR_HIP 0   // Front Right Hip
#define FL_HIP 1   // Front Left Hip
#define FR_LEG 2   // Front Right Leg
#define FL_LEG 3   // Front Left Leg
#define BR_HIP 4   // Back Right Hip
#define BL_HIP 5   // Back Left Hip
#define BR_LEG 6   // Back Right Leg
#define BL_LEG 7   // Back Left Leg

// Servo limits
#define SERVOMIN 125
#define SERVOMAX 575
#define SERVO_FREQ 50

class MiniKame_PCA {
  public:
    MiniKame_PCA();
    void init();

    // Basic movements
    void home();
    void walk(int steps, int period = 550, int dir = 1);
    void run(int steps, int period = 350, int dir = 1);
    void turnLeft(int steps, int period = 550);
    void turnRight(int steps, int period = 550);

    // Special movements
    void dance(int steps, int period = 2000);
    void moonwalk(int steps, int period = 2000);
    void pushUp(int steps, int period = 3000);
    void upDown(int steps, int period = 500);
    void frontBack(int steps, int period = 1000);
    void hello();
    void jump();

    // Utility
    void setServo(int ch, int angle);
    void setTrim(int ch, int value);
    int getTrim(int ch);
    void setSpeed(int speed);

  private:
    Adafruit_PWMServoDriver pwm;

    // Home positions (calibrated values)
    int homeHip[4] = {70, 110, 110, 70};  // FR, FL, BR, BL
    int homeLeg = 90;

    // Trim values for fine adjustment
    int trim[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    // Speed multiplier (100 = normal)
    int speedFactor = 100;

    // Helper functions
    void moveServos(int time, int target[8]);
    void oscillate(int steps, int period, int amplitude[8], int offset[8], int phase[8]);
    void delay_ms(int ms);
};

#endif
