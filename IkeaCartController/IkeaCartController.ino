/* 
*/

#include <IRremote.h>

#define PWM_1       6
#define PWM_2       5

#define FORWARD_1   4
#define REVERSE_1   3
#define FORWARD_2   A5
#define REVERSE_2   A4

#define IR_IN       7

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
  }
}

#define LEFT true
#define RIGHT false
#define FORWARD true
#define REVERSE false

void move(boolean left, boolean forward, int power) {
  int f = forward ? HIGH : LOW;
  int r = forward ? LOW : HIGH;
  if (left) {
    digitalWrite(FORWARD_1, f);
    digitalWrite(REVERSE_1, r);
    analogWrite(PWM_1, power);
  } else {
    digitalWrite(FORWARD_2, f);
    digitalWrite(REVERSE_2, r);
    analogWrite(PWM_2, power);
  }
}

void forward() {
  move(LEFT, FORWARD, 255);
  move(RIGHT, FORWARD, 255);
}

void reverse() {
  move(LEFT, REVERSE, 255);
  move(RIGHT, REVERSE, 255);
}

void left() {
  move(LEFT, REVERSE, 255);
  move(RIGHT, FORWARD, 255);
}

void right() {
  move(LEFT, FORWARD, 255);
  move(RIGHT, REVERSE, 255);
}

void stop() {
  analogWrite(PWM_1, 0);
  analogWrite(PWM_2, 0);
}

