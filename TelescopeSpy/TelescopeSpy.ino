/*
  Multiple Serial test

  Receives from the main serial port, sends to the others.
  Receives from serial port 1, sends to the main serial (Serial 0).

  This example works only with boards with more than one serial like Arduino Mega, Due, Zero etc.

  The circuit:
  - any serial device attached to Serial port 1
  - Serial Monitor open on Serial port 0

  created 30 Dec 2008
  modified 20 May 2012
  by Tom Igoe & Jed Roach
  modified 27 Nov 2015
  by Arturo Guadalupi

  This example code is in the public domain.
*/


void setup() {
  // initialize both serial ports:
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial2.begin(9600);
}

#define BUF_SIZE   50
char buffer[BUF_SIZE];

void readFrom(Stream *s1, Stream *s2, int n) {
  while (!s2->available()) ;
  if (s1->available()) {
    size_t len = s1->available();
    uint8_t sbuf[len];
    s1->readBytes(sbuf, len);
    Serial.print(n);
    Serial.print(",");
    Serial.print(len);
    for (uint8_t i = 0; i< len; i++) {
      sprintf(buffer, ",0x%02x", sbuf[i]);
      Serial.print(buffer);
    }
    Serial.println("");
  }
}

void relay(Stream *s1, Stream *s2) {
  int available;
  while (available = s1->available()) {
    available = min(BUF_SIZE, available);
    s1->readBytes(buffer, available);
    s2->write(buffer, available);
    s2->flush();
  }
}

void loop() {
//  readFrom(&Serial1, &Serial2, 1);
//  readFrom(&Serial2, &Serial1, 2);

  relay(&Serial, &Serial2);
  relay(&Serial2, &Serial);
}
