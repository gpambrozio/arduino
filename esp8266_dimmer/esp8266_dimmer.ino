
#define outputPin  2
#define zerocross  3
#define debugPin  1

int outVal = 50;
int direction = 0;

int timeoutPin = 414; // 87us

static long lastZC = 0;
static long thisZC = 0;

static bool waiting = false;
void ICACHE_RAM_ATTR isr_ext()
{
  if (outVal < 100) {
    digitalWrite(outputPin, LOW);
    timer1_write(timeoutPin * (100 - outVal));
    waiting = true;
  } else {
    digitalWrite(outputPin, HIGH);
    timer1_write(timeoutPin);
    waiting = false;
  }
  
  digitalWrite(debugPin, !digitalRead(debugPin));
  detachInterrupt(digitalPinToInterrupt(zerocross));
}

void ICACHE_RAM_ATTR onTimerISR()
{
  if (waiting) {
    digitalWrite(outputPin, HIGH);
    timer1_write(timeoutPin);
    waiting = false;
  } else {
    digitalWrite(outputPin, LOW);
    attachInterrupt(digitalPinToInterrupt(zerocross), isr_ext, FALLING);
  }
}

void setup() {
  digitalWrite(outputPin, LOW);
  pinMode(outputPin, OUTPUT);
  digitalWrite(debugPin, LOW);
  pinMode(debugPin, OUTPUT);
  pinMode(zerocross, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(zerocross), isr_ext, FALLING);
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
}

void loop() {
  outVal += direction;
  if (outVal >= 100) {
    direction = -1;
  } else if (outVal <= 1) {
    direction = 1;
  }

  delay(100);
}
