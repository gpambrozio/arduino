#define kFirstPin 26
#define kDelta    2
#define kOutputs  14
#define kLastPin  (kFirstPin + (kOutputs * kDelta))

void setup() {
  // put your setup code here, to run once:
  for (int i=kFirstPin; i<kLastPin; i+=kDelta) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
}

void loop() {
  // put your main code here, to run repeatedly: 
  for (int i=kFirstPin+4; i<kLastPin; i+=kDelta) {
    digitalWrite(i, HIGH);
    digitalWrite(i+kDelta, HIGH);
    delay(100);
    digitalWrite(i, LOW);
  }
  for (int i=kLastPin-kDelta; i>=kFirstPin; i-=kDelta) {
    digitalWrite(i, HIGH);
    digitalWrite(i-kDelta, HIGH);
    delay(100);
    digitalWrite(i, LOW);
  }
}
