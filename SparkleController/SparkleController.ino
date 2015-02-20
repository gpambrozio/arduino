/* 
*/

#include <Stepper.h>

Stepper myStepper = Stepper(64*8, 0, 2, 1, 3);

void setup() {
  myStepper.setSpeed(80);  // rpm
}

void loop() {
    myStepper.step(1024);
    delay(60000);
    myStepper.step(-1024);
    delay(60000);
}

