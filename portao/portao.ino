/*
  
 */
 
const int OUT1 = 11;
const int OUT2 = 12;

const int LED = 13;

// limite para o portao abrir, em segundos
const int LIMITE_ABERTURA = 15;

// limite entre um sinal e outro, em milisegundos
const int LIMITE_SINAL = 30 * 1000;

// limite que o portao pode ficar aberto, em segundos.
const int LIMITE_ABERTO = 3 * 60;

// Consts for the sensorState array
const int IN1 = 0;
const int IN2 = 1;

const int SENS1 = 2;
const int SENS2 = 3;

const int KEY = 4;

const byte SENSORS = 5;

// The pins
const int sensors[] = {2,4,8,7,10};
byte sensorState[SENSORS] = {0xFF,0xFF,0xFF,0xFF};

unsigned long debounce[SENSORS] = {0,0,0,0,0};
byte debounceState[SENSORS] = {0xFF,0xFF,0xFF,0xFF,0xFF};
byte reportState[SENSORS]   = {0xFF,0xFF,0xFF,0xFF,0xFF};

const int ON = 0x00;
const int OFF = 0xFF;

unsigned long lastSensorCheck = 0;

void checkStates()
{
  if (millis() - lastSensorCheck > 50)
  {
    lastSensorCheck = millis();
    for (int i=0; i<SENSORS; i++)
    {
      int val = digitalRead(sensors[i]);
      byte state = sensorState[i];
      state <<= 1;
      if (val == HIGH)
      {
        state |= 1;
      }
      sensorState[i] = state;
    }
    
    for (int i=0; i<2; i++)
    {
      if (sensorState[i] == ON)
      {
        debounceState[i] = ON;
        debounce[i] = millis();
      }
      else if (debounceState[i] == ON && millis() - debounce[i] > 5000)
      {
        debounceState[i] = OFF;
      }
    }
    
    for (int i=2;i<SENSORS;i++)
    {
      if (sensorState[i] == ON)
      {
        debounceState[i] = ON;
      }
      else if (sensorState[0] == OFF)
      {
        debounceState[i] = OFF;
      }
    }
  }
}

unsigned long lastLedBlink = 0;
int ledPeriod = 5000;
byte ledState;

void checkLed()
{
  if (millis() - lastLedBlink > ledPeriod)
  {
    lastLedBlink = millis();
    if (ledState == HIGH)
      ledState = LOW;
    else
      ledState = HIGH;
      
    digitalWrite(LED, ledState);
  }
}

byte ignoreSensors = 0;

void checkViolations()
{
  if (ignoreSensors & 0x01 == 0)
  {
    if (debounceState[SENS1] == OFF)
    {
      Serial.println("Portao 1 abriu sem comando!!!!!!");
      // TODO: Soar alarme
    }
  }
      
  if (ignoreSensors & 0x02 == 0)
  {
    if (debounceState[SENS2] == OFF)
    {
      Serial.println("Portao 2 abriu sem comando!!!!!!");
      // TODO: Soar alarme
    }
  }
}

unsigned long lastStateReport = 0;

void reportStates()
{
  if (millis() - lastStateReport > 1000)
  {
    lastStateReport = millis();
    boolean report = false;
    for (int i=0; i<SENSORS; i++)
    {
      if (debounceState[i] != reportState[i])
      {
        report = true;
        reportState[i] = debounceState[i];
      }
    }
    
    if (report)
    {
      for (int i=0; i<SENSORS; i++)
      {
        Serial.print(debounceState[i]);
        Serial.print(" ");
      }
      Serial.println("");
    }
  }
}

void checkAll()
{
  checkStates();
  checkLed();
  checkViolations();
  reportStates();
}

// Timeout is in seconds
boolean waitUntil(byte sensor, byte state, int timeout, boolean forever)
{
  if (!forever && timeout <= 0)
    return false;
  
  unsigned long millistimeout = 1000l * timeout;
  unsigned long start = millis();
  while (forever || (millis() - start) < millistimeout)
  {
    checkAll();
    if (debounceState[sensor] == state)
      return true;
//    if ( debounceState[KEY] == ON ) 
//      return false;
  }
  return false;
}

void setup()
{
  pinMode(OUT1, OUTPUT);
  digitalWrite(OUT1, LOW);
  pinMode(OUT2, OUTPUT);
  digitalWrite(OUT2, LOW);
  pinMode(LED, OUTPUT);
  for (int i=0; i<SENSORS; i++) {
    pinMode(sensors[i], INPUT);
    digitalWrite(sensors[i], HIGH); 
  }

  //start serial connection
  Serial.begin(9600);

  Serial.println("Versao 0002");
  ignoreSensors = 0x03;

  unsigned long start = millis();
  while ( (millis() - start) < 2000 ) {
    checkAll();
  }
  
  //Serial.println("Waiting sens1");
  //waitUntil(SENS1, ON, 0, true);
  Serial.println("Waiting sens2");
  waitUntil(SENS2, ON, 0, true);
  Serial.println("Started!");
  ignoreSensors = 0;
  ledPeriod = 3000;
}

void openAndWait(int out, byte sensor, byte portao)
{
  Serial.print("Abrindo portao ");
  Serial.println(portao);
  ledPeriod = 1000;
  unsigned long start = millis();
  while (true)
  {
    digitalWrite(out, HIGH);
    int limite = LIMITE_ABERTURA - (int)((millis() - start) / 1000); 
    if ( sensor == SENS1 )
      limite = 1; 
    if (!waitUntil(sensor, OFF, limite , false))
    {
      digitalWrite(out, LOW);
      Serial.println("Portao NAO ABRIU!");
      break;
    }
    else
    {
      delay(1000);
      digitalWrite(out, LOW);
      if (waitUntil(sensor, ON, 2, false))
      {
        Serial.println("debounce");
        continue;
      }

      ledPeriod = 500;
      Serial.println("Portao aberto");
      break;
    }
  }

  while (true)
  {
    if ( sensor == SENS1 )
        break;

    if (waitUntil(sensor, ON, LIMITE_ABERTO, false))
    {
      Serial.println("Portao fechou");
      break;
    }
    else 
    {
      ledPeriod = 200;
      Serial.println("Portao NAO FECHOU!");
      // TODO: soar alarme aqui!
    }
  }
  
  ledPeriod = 3000;
}

unsigned long lastSignal1 = 0;
unsigned long lastSignal2 = 0;

void loop()
{
  checkAll();
  if (debounceState[IN1] == ON && (millis() - lastSignal1) > LIMITE_SINAL)
  {
    ignoreSensors = 1;
    openAndWait(OUT1, SENS1, 1);
    ignoreSensors = 0;
    waitUntil(IN1, OFF, 0, true);
    lastSignal1 = millis();
  }
  else if (debounceState[IN2] == ON && (millis() - lastSignal2) > LIMITE_SINAL)
  {
    ignoreSensors = 2;
    openAndWait(OUT2, SENS2, 2);
    ignoreSensors = 0;
    waitUntil(IN2, OFF, 0, true);
    lastSignal2 = millis();
  }
/*  else if ( debounceState[KEY] == ON ) {
    ignoreSensors = 1;
    openAndWait(OUT1, SENS1, 1);
    ignoreSensors = 0;
    waitUntil(IN1, OFF, 0, true);
    lastSignal1 = millis();
  }
  */
}

