#define BLINKY_DIAG        0
#define CUBE_DIAG          0
#define COMM_LED_PIN       2
#define RST_BUTTON_PIN     3
#include <BlinkyPicoW.h>
#include <SPI.h>

struct CubeSetting
{
  uint16_t publishInterval;
  int16_t switchOpenAlarm;
};
CubeSetting setting;

struct CubeReading
{
  int16_t switchState[3];
};
CubeReading reading;

unsigned long lastPublishTime;

int powerPin[]  = {10, 13, 16};
int switchPin[] = {11, 14, 17};
int signalPin[] = {12, 15, 18};
unsigned long switchTime[] = {0, 0, 0};

unsigned long debounceInterval = 20;
boolean forceArchiveData = false;

void setupBlinky()
{
  if (BLINKY_DIAG > 0) Serial.begin(9600);

  BlinkyPicoW.setMqttKeepAlive(15);
  BlinkyPicoW.setMqttSocketTimeout(4);
  BlinkyPicoW.setMqttPort(1883);
  BlinkyPicoW.setMqttLedFlashMs(100);
  BlinkyPicoW.setHdwrWatchdogMs(8000);

  BlinkyPicoW.begin(BLINKY_DIAG, COMM_LED_PIN, RST_BUTTON_PIN, true, sizeof(setting), sizeof(reading));
}


void setupCube()
{
  if (CUBE_DIAG > 0) Serial.begin(9600);
  setting.publishInterval = 2000;
  lastPublishTime = millis();
  
  for (int ii = 0; ii < 3; ++ii)
  {
    pinMode(powerPin[ii], OUTPUT);
    pinMode(switchPin[ii], INPUT);
    pinMode(signalPin[ii], OUTPUT);
    digitalWrite(powerPin[ii], HIGH);
    digitalWrite(signalPin[ii], LOW);
    reading.switchState[ii] = -1;
  }

  setting.switchOpenAlarm = 1;
  forceArchiveData = false;
}

void loopCube()
{
  unsigned long now = millis();
  boolean stateChange = false;
  for (int ii = 0; ii < 3; ++ii)
  {
    int16_t pinValue = (int16_t) digitalRead(switchPin[ii]);
    if (pinValue != reading.switchState[ii])
    {
      if((now - switchTime[ii]) > debounceInterval)
      {
        reading.switchState[ii] = pinValue;
        switchTime[ii] = now;
        stateChange = true;
        boolean signalVal = false;
        if (pinValue > 0) signalVal = true;
        if (setting.switchOpenAlarm == 1) signalVal = !signalVal;
        digitalWrite(signalPin[ii], signalVal);
      }
    }
  }
  forceArchiveData = false;
  if (stateChange)
  {
    forceArchiveData = true;
    publishData(now);  
  }
  if ((now - lastPublishTime) > setting.publishInterval)
  {
    publishData(now);
  }
  boolean newSettings = BlinkyPicoW.retrieveCubeSetting((uint8_t*) &setting);
  if (newSettings)
  {
    if (setting.publishInterval < 1000) setting.publishInterval = 1000;
  }
}

void publishData(unsigned long now)
{
  lastPublishTime = now;
  
  boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, forceArchiveData);
  if (CUBE_DIAG > 0)
  {
    Serial.print(reading.switchState[0]);
    Serial.print(",");
    Serial.print(reading.switchState[1]);
    Serial.print(",");
    Serial.println(reading.switchState[2]);
  }
}
