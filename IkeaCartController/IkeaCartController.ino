/* 
*/

#include <Stepper.h>
#include <IRremote.h>

#define PWM_1       6
#define PWM_2       5

#define FORWARD_1   4
#define REVERSE_1   3
#define FORWARD_2   A5
#define REVERSE_2   A4

#define IR_IN       7

/*-----( Declare Constants, Pin Numbers )-----*/
//---( Number of steps per revolution of INTERNAL motor in 4-step mode )---
#define STEPS_PER_MOTOR_REVOLUTION 32   

//---( Steps per OUTPUT SHAFT of gear reduction )---
#define STEPS_PER_OUTPUT_REVOLUTION 32 * 64  //2048

Stepper myStepper = Stepper(STEPS_PER_MOTOR_REVOLUTION, 12, 10, 11, 9);

/* Define the IR remote button codes. We're only using the
   least signinficant two bytes of these codes. Each one 
   should actually have 0x10EF in front of it. Find these codes
   by running the IRrecvDump example sketch included with
   the IRremote library.*/
#define BUTTON_POWER  0xD827
#define BUTTON_A      0xF807
#define BUTTON_B      0x7887
#define BUTTON_C      0x58A7
#define BUTTON_UP     0xA05F
#define BUTTON_DOWN   0x00FF
#define BUTTON_LEFT   0x10EF
#define BUTTON_RIGHT  0x807F
#define BUTTON_CIRCLE 0x20DF

IRrecv irrecv(IR_IN);

void setup() {
  Serial.begin(115200);  

  myStepper.setSpeed(500);  // rpm
  irrecv.enableIRIn(); // Start the receiver

  digitalWrite(PWM_1, LOW);
  digitalWrite(PWM_2, LOW);
  digitalWrite(FORWARD_1, LOW);
  digitalWrite(REVERSE_1, LOW);
  digitalWrite(FORWARD_2, LOW);
  digitalWrite(REVERSE_2, LOW);

  pinMode(PWM_1, OUTPUT);
  pinMode(PWM_2, OUTPUT);
  pinMode(FORWARD_1, OUTPUT);
  pinMode(REVERSE_1, OUTPUT);
  pinMode(FORWARD_2, OUTPUT);
  pinMode(REVERSE_2, OUTPUT);
}

unsigned long lastMove = 0;
bool isDown = false;
decode_results results;
uint16_t lastCode = 0;
unsigned long lastCodeTime = 0;

void loop() {
  if (irrecv.decode(&results)) 
  {
    /* read the RX'd IR into a 16-bit variable: */
    uint16_t resultCode = (results.value & 0xFFFF);
    irrecv.resume(); // Receive the next value

    /* The remote will continue to spit out 0xFFFFFFFF if a 
     button is held down. If we get 0xFFFFFFF, let's just
     assume the previously pressed button is being held down */
    if (resultCode == 0xFFFF)
      resultCode = lastCode;
    else
      lastCode = resultCode;

    lastCodeTime = millis();

    switch (resultCode)
    {
      case BUTTON_POWER:
        Serial.println("Power");
        retract();
        break;
      case BUTTON_A:
        Serial.println("A");
        break;
      case BUTTON_B:
        Serial.println("B");
        break;
      case BUTTON_C:
        Serial.println("C");
        break;
      case BUTTON_UP:
        Serial.println("Up");
        forward();
        break;
      case BUTTON_DOWN:
        Serial.println("Down");
        reverse();
        break;
      case BUTTON_LEFT:
        Serial.println("Left");
        left();
        break;
      case BUTTON_RIGHT:
        Serial.println("Right");
        right();
        break;
    }
  } else if (lastCodeTime > 0 && millis() - lastCodeTime > 200) {
    Serial.println("Stop");
    stop();
    lastCodeTime = 0;
    lastMove = millis();
  } else if (lastMove > 0 && millis() - lastMove > 5000) {
    lastMove = 0;
    retract();
  }
}

void lower() {
  if (!isDown) {
    isDown = true;
//    myStepper.step(STEPS_PER_OUTPUT_REVOLUTION/4);
  }
}

void retract() {
  if (isDown) {
    isDown = false;
//    myStepper.step(-STEPS_PER_OUTPUT_REVOLUTION/4);
  }
}

void move(boolean motor1, boolean reverse) {
  lower();
  lastMove = millis();
  
  int f = reverse ? HIGH : LOW;
  int r = reverse ? LOW : HIGH;
  if (motor1) {
    digitalWrite(FORWARD_1, f);
    digitalWrite(REVERSE_1, r);
    analogWrite(PWM_1, 255);
  } else {
    digitalWrite(FORWARD_2, f);
    digitalWrite(REVERSE_2, r);
    analogWrite(PWM_2, 255);
  }
}

void forward() {
  move(true, true);
  move(false, true);
}

void reverse() {
  move(true, false);
  move(false, false);
}

void right() {
  move(true, true);
  move(false, false);
}

void left() {
  move(true, false);
  move(false, true);
}

void stop() {
  analogWrite(PWM_1, 0);
  analogWrite(PWM_2, 0);
}

