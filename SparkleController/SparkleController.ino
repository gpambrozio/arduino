/* 
*/

#include <Stepper.h>

Stepper myStepper = Stepper(64*8, 0, 3, 1, 4);

void setup() {
  myStepper.setSpeed(80);  // rpm
}

void loop() {
    myStepper.step(1024);
    delay(60000);
}

