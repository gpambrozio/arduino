/*****************************************************************************
 ****************************************************************************/

#define EXTRA_GND_1     38
#define EXTRA_GND_2     39

#define FIRST_LIGHT   22
#define NUMBER_OF_RELAYS 16

#define MODE_BUTTON    6
#define AUDIO_IN       A0
#define LEVEL_IN       A1

#define NOISE         10

#define NUMBER_OF_LIGHTS 6

#define NOISE     10  // Noise/hum/interference in mic signal
#define SAMPLES   36  // Length of buffer for dynamic level adjustment
#define TOP       (NUMBER_OF_LIGHTS + 2) // Allow dot to go slightly off scale

typedef enum {
  ModeSound = 0,
  ModePulse,
  ModeBounce,
  ModeOn,
  ModeCount
} Mode;

Mode currentMode = ModePulse;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");
  
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  
  digitalWrite(EXTRA_GND_1, LOW);
  pinMode(EXTRA_GND_1, OUTPUT);
  digitalWrite(EXTRA_GND_2, LOW);
  pinMode(EXTRA_GND_2, OUTPUT);

  for(int x=0; x<NUMBER_OF_RELAYS; x++) {
    digitalWrite(FIRST_LIGHT+x, HIGH);
    pinMode(FIRST_LIGHT+x, OUTPUT);
  }
}

void loop1() {
  for(int x=0; x<NUMBER_OF_RELAYS; x++) {
    digitalWrite(FIRST_LIGHT+x, LOW);
    delay(2000);
    digitalWrite(FIRST_LIGHT+x, HIGH);
  }
  delay(2000);
}

int audio;
unsigned long buttonStart = 0;
unsigned long span;

int lvl = 0;
int vol[SAMPLES];
int volCount;
int minLvlAvg, maxLvlAvg;
int minLvl, maxLvl;

int level;

void loop() {
  level = ((level * 7) + max(20, analogRead(LEVEL_IN))) >> 3;
  
  if (digitalRead(MODE_BUTTON) == LOW && millis() > buttonStart + 500) {
    buttonStart = millis();
    currentMode = (Mode)((int)currentMode + 1);
    if (currentMode >= ModeCount) currentMode = (Mode)0;
  }
  
  audio   = analogRead(AUDIO_IN);         // Raw reading from mic
  audio   = abs(audio - 512);             // Center on zero
  audio   = (audio <= NOISE) ? 0 : (audio - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + audio) >> 3;    // "Dampened" reading (else looks twitchy)
  
  vol[volCount] = audio;                 // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  audio = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if(audio < 0L)       audio = 0;      // Clip output
  else if(audio > TOP) audio = TOP;

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];

  for(int i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl)      minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)

  switch(currentMode) {
    case ModeSound:
      for(int i = 0; i < NUMBER_OF_LIGHTS; i++) {
        digitalWrite(FIRST_LIGHT + i, i < audio ? LOW : HIGH);
      }
      break;
      
    case ModeOn:
      for(int i = 0; i < NUMBER_OF_LIGHTS; i++) {
        digitalWrite(FIRST_LIGHT + i, i < HIGH);
      }
      break;
      
    case ModePulse:
    {
      span = millis();
      span %= level * 2;
      int state = HIGH;
      if (span < level)
        state = LOW;
      for(int i = 0; i < NUMBER_OF_LIGHTS; i++) {
        digitalWrite(FIRST_LIGHT + i, state);
      }
      break;
    }
      
    case ModeBounce:
      span = millis();
      span %= (level * (NUMBER_OF_LIGHTS * 2 - 2));
      span /= level;
      for(int i = 0; i < NUMBER_OF_LIGHTS; i++) {
        digitalWrite(FIRST_LIGHT + i, (i == span || i == NUMBER_OF_LIGHTS * 2 - 2 - span) ? LOW : HIGH);
      }
      break;
      
  }
  delay(4);
}

