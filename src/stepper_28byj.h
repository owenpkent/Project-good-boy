#pragma once
#include <Arduino.h>

// Simple 28BYJ-48 stepper driver for ULN2003 board (half-step sequence)
class Stepper28BYJ {
public:
  Stepper28BYJ(int in1, int in2, int in3, int in4, unsigned int stepDelayMicros = 1200)
  : p1(in1), p2(in2), p3(in3), p4(in4), delayUs(stepDelayMicros) {}

  void begin() {
    pinMode(p1, OUTPUT);
    pinMode(p2, OUTPUT);
    pinMode(p3, OUTPUT);
    pinMode(p4, OUTPUT);
    release();
  }

  // Set delay between half-steps (us). Larger = slower, more torque.
  void setDelayMicros(unsigned int us) { delayUs = us; }

  // Steps > 0 rotates forward; < 0 rotates backward
  void step(long steps) {
    if (steps == 0) return;
    const int dir = (steps > 0) ? 1 : -1;
    steps = labs(steps);
    while (steps--) {
      phase = (phase + dir) & 7; // wrap 0..7
      applyPhase(phase);
      delayMicroseconds(delayUs);
    }
    // Optional: hold torque by leaving coils energized; or call release() to save power
    release();
  }

  // Energize coils per half-step phase
  void applyPhase(uint8_t ph) {
    // Half-step 8-phase sequence
    static const uint8_t seq[8][4] = {
      {1,0,0,0},
      {1,1,0,0},
      {0,1,0,0},
      {0,1,1,0},
      {0,0,1,0},
      {0,0,1,1},
      {0,0,0,1},
      {1,0,0,1}
    };
    digitalWrite(p1, seq[ph][0]);
    digitalWrite(p2, seq[ph][1]);
    digitalWrite(p3, seq[ph][2]);
    digitalWrite(p4, seq[ph][3]);
  }

  // De-energize coils (reduce holding torque and save power)
  void release() {
    digitalWrite(p1, LOW);
    digitalWrite(p2, LOW);
    digitalWrite(p3, LOW);
    digitalWrite(p4, LOW);
  }

private:
  int p1, p2, p3, p4;
  unsigned int delayUs;
  int phase = 0;
};
