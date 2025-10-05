#include <stepper_28byj.h>

void setup() {
  Serial.begin(115200);
  Serial.println("CLI stepper compile test");
  Stepper28BYJ stepper(14, 27, 26, 25, 1200);
  stepper.begin();
  // No motion here; compile-only test without hardware attached
}

void loop() {}
