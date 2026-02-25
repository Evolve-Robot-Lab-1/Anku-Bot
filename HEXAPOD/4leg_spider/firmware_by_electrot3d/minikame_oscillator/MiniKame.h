/*
 * MiniKame for PCA9685
 * Oscillator-based smooth movement
 * Adapted for your servo configuration
 */

#ifndef MINIKAME_H
#define MINIKAME_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "Oscillator.h"

// Servo channels
#define FR_HIP 0
#define FL_HIP 1
#define FR_LEG 2
#define FL_LEG 3
#define BR_HIP 4
#define BL_HIP 5
#define BR_LEG 6
#define BL_LEG 7

// PCA9685 settings
#define SERVOMIN 125
#define SERVOMAX 575
#define SERVO_FREQ 50

class MiniKame {
  public:
    MiniKame();
    void init();

    // Basic movements
    void home();
    void walk(int dir, float steps, float T = 550);
    void run(int dir, float steps, float T = 550);
    void turnL(float steps, float T = 550);
    void turnR(float steps, float T = 550);

    // Fun movements
    void dance(float steps, float T = 2000);
    void moonwalkL(float steps, float T = 2000);
    void upDown(float steps, float T = 500);
    void pushUp(float steps, float T = 5000);
    void frontBack(float steps, float T = 2000);
    void hello();
    void jump();

    // Utility
    void setServo(int id, float angle);
    void setTrim(int id, int value);
    int getTrim(int id);
    void refresh();

  private:
    Adafruit_PWMServoDriver pwm;
    Oscillator oscillator[8];
    int trim[8];

    void execute(float steps, int period[8], int amplitude[8], int offset[8], int phase[8]);
    void moveServos(int time, float target[8]);
};

#endif
