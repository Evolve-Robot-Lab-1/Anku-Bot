/*
 * Oscillator class for smooth sinusoidal servo movement
 * Ported for PCA9685 (no Servo.h dependency)
 */

#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <Arduino.h>

#ifndef PI
  #define PI 3.14159265359
#endif

class Oscillator {
  public:
    Oscillator();
    float refresh();
    void reset();
    void start();
    void start(unsigned long ref_time);
    void stop();
    boolean isStop();

    void setPeriod(int period);
    void setAmplitude(int amplitude);
    void setPhase(int phase);
    void setOffset(int offset);
    void setTime(unsigned long ref);

    int getPeriod();
    float getOutput();
    float getPhaseProgress();
    unsigned long getTime();

  private:
    int _period;
    int _amplitude;
    int _phase;
    int _offset;
    float _output;
    bool _stop;
    unsigned long _ref_time;
    float _delta_time;

    float time_to_radians(double time);
    float degrees_to_radians(float degrees);
};

#endif
