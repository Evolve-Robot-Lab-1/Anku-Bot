#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ---------- Struct ----------
struct LegPose {
  int coxa;
  int femur;
  int tibia;
};

// ---------- TWO PCA9685 BOARDS ----------
Adafruit_PWMServoDriver pca0 = Adafruit_PWMServoDriver(0x40); // Board 0
Adafruit_PWMServoDriver pca1 = Adafruit_PWMServoDriver(0x41); // Board 1 (A0 bridged)

// Correct timing for hobby servos (roughly 1ms..2ms)
#define SERVOMIN 205    // ~1 ms  (0°)
#define SERVOMAX 410    // ~2 ms (180°)

uint16_t angleToPulse(int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

// ---------- CONFIG ----------

const uint8_t NUM_LEGS = 6;   // 0..5
// joint index: 0 = coxa, 1 = femur, 2 = tibia

// Which board each leg’s joints are on: 0 = pca0 (0x40), 1 = pca1 (0x41)
const uint8_t JOINT_BOARD[NUM_LEGS][3] = {
  {0,0,0},  // leg 0 on board 0x40
  {0,0,0},  // leg 1 on board 0x40
  {0,0,0},  // leg 2 on board 0x40
  {1,1,1},  // leg 3 on board 0x41
  {1,1,1},  // leg 4 on board 0x41
  {1,1,1}   // leg 5 on board 0x41
};

// Channel mapping on each board (0..15)
const uint8_t JOINT_CH[NUM_LEGS][3] = {
  {0,1,2},  // leg 0: coxa0, femur1, tibia2  (board 0)
  {3,4,5},  // leg 1: coxa3, femur4, tibia5  (board 0)
  {6,7,8},  // leg 2: coxa6, femur7, tibia8  (board 0)
  {0,1,2},  // leg 3: coxa0, femur1, tibia2  (board 1)
  {3,4,5},  // leg 4: coxa3, femur4, tibia5  (board 1)
  {6,7,8}   // leg 5: coxa6, femur7, tibia8  (board 1)
};

// ---------- POSES ----------
LegPose POSE_HOME      = { 90, 90, 90 };  // neutral
LegPose POSE_LIFT      = { 90, 75, 105 }; // femur up, tibia bent
LegPose POSE_SWING_FWD = {105, 75, 105 }; // coxa forward, still lifted
LegPose POSE_PLACE     = {105, 90,  90 }; // foot down forward

// Track current pose per leg
LegPose currentPose[NUM_LEGS] = {
  POSE_HOME, POSE_HOME, POSE_HOME,
  POSE_HOME, POSE_HOME, POSE_HOME
};

// ---------- RUN CONTROL ----------
bool running = false;  // false = stopped, true = walking

void checkSerial() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'p' || c == 'P') {
      running = true;
      Serial.println("Gait: START");
    } else if (c == 'c' || c == 'C') {
      running = false;
      Serial.println("Gait: STOP");
    }
  }
}

// ---------- LOW-LEVEL SERVO HELPERS ----------

void setJoint(uint8_t legIndex, uint8_t whichJoint, int angle) {
  uint8_t board = JOINT_BOARD[legIndex][whichJoint];
  uint8_t ch    = JOINT_CH[legIndex][whichJoint];

  uint16_t pulse = angleToPulse(angle);

  if (board == 0) {
    pca0.setPWM(ch, 0, pulse);
  } else {
    pca1.setPWM(ch, 0, pulse);
  }
}

void setLegPose(uint8_t legIndex, LegPose p) {
  setJoint(legIndex, 0, p.coxa);
  setJoint(legIndex, 1, p.femur);
  setJoint(legIndex, 2, p.tibia);
  currentPose[legIndex] = p;
}

// Smooth move one leg from one pose to another
void moveLegSmooth(uint8_t legIndex,
                   LegPose from, LegPose to,
                   int steps, int stepDelayMs) {
  for (int i = 0; i <= steps; i++) {
    checkSerial();
    if (!running) {
      // If stopped mid-move, gently go back to home and exit
      setLegPose(legIndex, POSE_HOME);
      return;
    }

    float t = (float)i / (float)steps;

    LegPose cur;
    cur.coxa  = from.coxa  + (int)((to.coxa  - from.coxa)  * t);
    cur.femur = from.femur + (int)((to.femur - from.femur) * t);
    cur.tibia = from.tibia + (int)((to.tibia - from.tibia) * t);

    setLegPose(legIndex, cur);
    delay(stepDelayMs);
  }
}

// ---------- GAIT FOR SINGLE LEG ----------

void singleLegGait(uint8_t legIndex) {
  const int steps     = 12;  // smaller steps = faster
  const int stepDelay = 12;  // smaller delay = faster

  // Make sure we start from HOME
  moveLegSmooth(legIndex, currentPose[legIndex], POSE_HOME, steps, stepDelay);
  if (!running) return;

  // 1) LIFT
  moveLegSmooth(legIndex, POSE_HOME, POSE_LIFT, steps, stepDelay);
  if (!running) return;

  // 2) SWING FORWARD
  moveLegSmooth(legIndex, POSE_LIFT, POSE_SWING_FWD, steps, stepDelay);
  if (!running) return;

  // 3) PLACE DOWN
  moveLegSmooth(legIndex, POSE_SWING_FWD, POSE_PLACE, steps, stepDelay);
  if (!running) return;

  // 4) RETURN TO HOME
  moveLegSmooth(legIndex, POSE_PLACE, POSE_HOME, steps, stepDelay);
}

// ---------- SETUP & LOOP ----------

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("6-Leg (18 motor) Gait Test - 2x PCA9685");
  Serial.println("Press 'p' to START, 'c' to STOP");

  Wire.begin();          // SDA=A4, SCL=A5

  // Init both PCA boards
  pca0.begin();
  pca0.setPWMFreq(50);

  pca1.begin();
  pca1.setPWMFreq(50);

  delay(10);

  // Move all 18 motors to HOME = 90°
  for (uint8_t leg = 0; leg < NUM_LEGS; leg++) {
    setLegPose(leg, POSE_HOME);
    delay(100);
  }
}

void loop() {
  checkSerial();

  if (!running) {
    // Keep legs at HOME when stopped
    for (uint8_t leg = 0; leg < NUM_LEGS; leg++) {
      setLegPose(leg, POSE_HOME);
    }
    delay(50);
    return;
  }

  // Gait: Leg 0 -> 1 -> 2 -> 3 -> 4 -> 5
  for (uint8_t leg = 0; leg < NUM_LEGS; leg++) {
    Serial.print("Leg ");
    Serial.print(leg);
    Serial.println(" gait...");
    singleLegGait(leg);
    if (!running) return;
  }
}