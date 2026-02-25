/*
 * MiniKame for PCA9685 - Implementation (FIXED)
 */

#include "MiniKame.h"

MiniKame::MiniKame() : pwm() {
  for (int i = 0; i < 8; i++) {
    trim[i] = 0;
  }
}

void MiniKame::init() {
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
  home();
}

void MiniKame::setServo(int id, float angle) {
  angle = angle + trim[id];
  angle = constrain(angle, 0, 180);
  int pulse = map((int)angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(id, 0, pulse);
}

void MiniKame::setTrim(int id, int value) {
  trim[id] = constrain(value, -30, 30);
}

int MiniKame::getTrim(int id) {
  return trim[id];
}

void MiniKame::home() {
  // YOUR working home positions from walk_test
  setServo(FR_HIP, 70);
  setServo(FL_HIP, 110);
  setServo(BR_HIP, 110);
  setServo(BL_HIP, 70);

  setServo(FR_LEG, 90);
  setServo(FL_LEG, 90);
  setServo(BR_LEG, 90);
  setServo(BL_LEG, 90);

  for (int i = 0; i < 8; i++) {
    oscillator[i].stop();
  }
}

void MiniKame::walk(int dir, float steps, float T) {
  /*
   * From YOUR walk_test servo directions:
   * Step1: FR+BL lift, all hips swing
   *   FR_HIP: 70->45 (decrease), FL_HIP: 110->85 (decrease)
   *   BR_HIP: 110->135 (increase), BL_HIP: 70->95 (increase)
   *   FR_LEG: 90->55 (decrease=lift), BL_LEG: 90->125 (increase=lift)
   *
   * Step2: FL+BR lift, hips swing back
   *   FR_HIP: 70->95 (increase), FL_HIP: 110->135 (increase)
   *   BR_HIP: 110->85 (decrease), BL_HIP: 70->45 (decrease)
   *   FL_LEG: 90->140 (increase=lift), BR_LEG: 90->40 (decrease=lift)
   */

  int hipSwing = 25;
  int legLift = 40;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};

  // All positive amplitudes, direction handled by phase
  int amplitude[] = {hipSwing, hipSwing, legLift, legLift, hipSwing, hipSwing, legLift, legLift};

  // Home positions
  int offset[] = {70, 110, 90, 90, 110, 70, 90, 90};

  // Phases (270=at min, 90=at max)
  // Step1: FR/FL hips at min, BR/BL hips at max
  //        FR_LEG at min (lift), BL_LEG at max (lift)
  int phase[] = {
    270,  // FR_HIP: at min in Step1
    270,  // FL_HIP: at min in Step1
    270,  // FR_LEG: at min=lift in Step1
    90,   // FL_LEG: at max=lift in Step2 (180° later)
    90,   // BR_HIP: at max in Step1
    90,   // BL_HIP: at max in Step1
    90,   // BR_LEG: at min=lift in Step2 (use 90+180=270? no, decrease=lift so 270)
    90    // BL_LEG: at max=lift in Step1
  };

  // Fix BR_LEG: decrease=lift, so lift is at min (270), but it lifts in Step2
  // Step2 is 180° from Step1, so BR_LEG phase = 270-180 = 90...
  // But wait, at phase 90 output is max, not min.
  // Let me just set BR_LEG to 90 (at max in Step1 = down), then at Step2 it's at 270 (min = lift)
  phase[6] = 90;  // BR_LEG: down in Step1, lift in Step2

  if (dir == 0) {  // Backward - invert hip phases
    phase[0] = 90; phase[1] = 90;
    phase[4] = 270; phase[5] = 270;
  }

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::run(int dir, float steps, float T) {
  int x_amp = 15;
  int z_amp = 15;
  int ap = 15;
  int hi = 0;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {x_amp, x_amp, z_amp, z_amp, x_amp, x_amp, z_amp, z_amp};
  int offset[] = {
    90 + ap, 90 - ap,
    90 - hi, 90 + hi,
    90 - ap, 90 + ap,
    90 + hi, 90 - hi
  };
  int phase[] = {0, 0, 90, 90, 180, 180, 90, 90};

  if (dir == 1) {
    phase[0] = phase[1] = 180;
    phase[4] = phase[5] = 0;
  }

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::turnR(float steps, float T) {
  int x_amp = 15;
  int z_amp = 15;
  int ap = 15;
  int hi = 0;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {x_amp, x_amp, z_amp, z_amp, x_amp, x_amp, z_amp, z_amp};
  int offset[] = {
    90 + ap, 90 - ap,
    90 - hi, 90 + hi,
    90 - ap, 90 + ap,
    90 + hi, 90 - hi
  };
  int phase[] = {0, 180, 90, 90, 180, 0, 90, 90};

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::turnL(float steps, float T) {
  int x_amp = 15;
  int z_amp = 15;
  int ap = 15;
  int hi = 0;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {x_amp, x_amp, z_amp, z_amp, x_amp, x_amp, z_amp, z_amp};
  int offset[] = {
    90 + ap, 90 - ap,
    90 - hi, 90 + hi,
    90 - ap, 90 + ap,
    90 + hi, 90 - hi
  };
  int phase[] = {180, 0, 90, 90, 0, 180, 90, 90};

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::dance(float steps, float T) {
  int x_amp = 0;
  int z_amp = 40;
  int ap = 30;
  int hi = 0;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {x_amp, x_amp, z_amp, z_amp, x_amp, x_amp, z_amp, z_amp};
  int offset[] = {
    90 + ap, 90 - ap,
    90 - hi, 90 + hi,
    90 - ap, 90 + ap,
    90 + hi, 90 - hi
  };
  int phase[] = {0, 0, 0, 270, 0, 0, 90, 180};

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::moonwalkL(float steps, float T) {
  int z_amp = 45;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {0, 0, z_amp, z_amp, 0, 0, z_amp, z_amp};
  int offset[] = {90, 90, 90, 90, 90, 90, 90, 90};
  int phase[] = {0, 0, 0, 120, 0, 0, 180, 290};

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::upDown(float steps, float T) {
  int x_amp = 0;
  int z_amp = 35;
  int ap = 20;
  int hi = 0;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {x_amp, x_amp, z_amp, z_amp, x_amp, x_amp, z_amp, z_amp};
  int offset[] = {
    90 + ap, 90 - ap,
    90 - hi, 90 + hi,
    90 - ap, 90 + ap,
    90 + hi, 90 - hi
  };
  int phase[] = {0, 0, 90, 270, 180, 180, 270, 90};

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::pushUp(float steps, float T) {
  int z_amp = 40;
  int x_amp = 65;
  int hi = 0;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {0, 0, z_amp, z_amp, 0, 0, 0, 0};
  int offset[] = {90, 90, 90 - hi, 90 + hi, 90 - x_amp, 90 + x_amp, 90 + hi, 90 - hi};
  int phase[] = {0, 0, 0, 180, 0, 0, 0, 180};

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::frontBack(float steps, float T) {
  int x_amp = 30;
  int z_amp = 25;
  int ap = 20;
  int hi = 0;

  int period[] = {(int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T, (int)T};
  int amplitude[] = {x_amp, x_amp, z_amp, z_amp, x_amp, x_amp, z_amp, z_amp};
  int offset[] = {
    90 + ap, 90 - ap,
    90 - hi, 90 + hi,
    90 - ap, 90 + ap,
    90 + hi, 90 - hi
  };
  int phase[] = {0, 180, 270, 90, 0, 180, 90, 270};

  execute(steps, period, amplitude, offset, phase);
}

void MiniKame::hello() {
  float seated[] = {90 + 15, 90 - 15, 90 - 65, 90 + 65, 90 + 20, 90 - 20, 90 + 10, 90 - 10};
  moveServos(150, seated);
  delay(200);

  int T = 350;
  int period[] = {T, T, T, T, T, T, T, T};
  int amplitude[] = {0, 50, 0, 50, 0, 0, 0, 0};
  int offset[] = {90 + 15, 40, 90 - 10, 90 + 10, 90 + 20, 90 - 20, 90 + 65, 90};
  int phase[] = {0, 0, 0, 90, 0, 0, 0, 0};

  execute(4, period, amplitude, offset, phase);

  float goingUp[] = {160, 20, 90, 90, 90 - 20, 90 + 20, 90 + 10, 90 - 10};
  moveServos(500, goingUp);
  delay(200);
}

void MiniKame::jump() {
  float seated[] = {
    90 + 15, 90 - 15,
    90 - 10, 90 + 10,
    90 + 10, 90 - 10,
    90 + 65, 90 - 65
  };

  int ap = 20;
  int hi = 35;
  float jump_pos[] = {90 + ap, 90 - ap, 90 - hi, 90 + hi, 90 - ap*3, 90 + ap*3, 90 + hi, 90 - hi};

  moveServos(150, seated);
  delay(200);
  moveServos(0, jump_pos);
  delay(100);
  home();
}

void MiniKame::execute(float steps, int period[8], int amplitude[8], int offset[8], int phase[8]) {
  // Setup and start all oscillators
  unsigned long ref = millis();

  for (int i = 0; i < 8; i++) {
    oscillator[i].setPeriod(period[i]);
    oscillator[i].setAmplitude(amplitude[i]);
    oscillator[i].setPhase(phase[i]);
    oscillator[i].setOffset(offset[i]);
    oscillator[i].setTime(ref);
    oscillator[i].start(ref);
  }

  // Run for duration
  unsigned long duration = (unsigned long)(period[0] * steps);
  unsigned long startTime = millis();

  while (millis() - startTime < duration) {
    for (int i = 0; i < 8; i++) {
      setServo(i, oscillator[i].refresh());
    }
    delay(10);
  }

  home();
}

void MiniKame::refresh() {
  for (int i = 0; i < 8; i++) {
    if (!oscillator[i].isStop()) {
      setServo(i, oscillator[i].refresh());
    }
  }
}

void MiniKame::moveServos(int time, float target[8]) {
  float current[8];

  for (int i = 0; i < 8; i++) {
    current[i] = 90;  // Assume starting from center
  }

  if (time > 10) {
    float increment[8];
    for (int i = 0; i < 8; i++) {
      increment[i] = (target[i] - current[i]) / (time / 10.0);
    }

    int steps = time / 10;
    for (int s = 0; s < steps; s++) {
      for (int i = 0; i < 8; i++) {
        current[i] += increment[i];
        setServo(i, current[i]);
      }
      delay(10);
    }
  }

  for (int i = 0; i < 8; i++) {
    setServo(i, target[i]);
  }
}
