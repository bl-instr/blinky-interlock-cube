#define BLINKY_DIAG        0
#define CUBE_DIAG          0
#define COMM_LED_PIN       2
#define RST_BUTTON_PIN     3
#include <BlinkyPicoW.h>

struct CubeSetting
{
  uint16_t publishInterval;
  uint16_t invert[3];
  uint16_t termination[3];
  uint16_t interlockOn[3];
};
CubeSetting setting;

struct CubeReading
{
  uint16_t switchState[3];
};
CubeReading reading;

unsigned long lastPublishTime;

int powerPin[]  = {10, 13, 16};
int switchPin[] = {11, 14, 17};
uint16_t oldTermination[] = {1, 1, 1};
uint16_t oldInterlockOn[] = {1, 1, 1};
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
    pinMode(switchPin[ii], INPUT_PULLDOWN);
    digitalWrite(powerPin[ii], HIGH);
    reading.switchState[ii] = -1;
    setting.invert[ii] = 0;
    setting.termination[ii] = 1;
    oldTermination[ii] = 1;
    oldInterlockOn[ii] = 1;
  }

  forceArchiveData = false;
}

void loopCube()
{
  unsigned long now = millis();
  boolean stateChange = false;
  for (int ii = 0; ii < 3; ++ii)
  {
    uint16_t pinValue = (uint16_t) digitalRead(switchPin[ii]);
    if (setting.invert[ii] > 0)
    {
      if (pinValue > 0)
      {
        pinValue = 0;
      }
      else
      {
        pinValue = 1;
      }
    }
    if (pinValue != reading.switchState[ii])
    {
      if((now - switchTime[ii]) > debounceInterval)
      {
        reading.switchState[ii] = pinValue;
        switchTime[ii] = now;
        stateChange = true;
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
    for (int ii = 0; ii < 3; ++ii)
    {
      if (setting.invert[ii] > 0)
      {
        setting.invert[ii] = 1;
      }
      else
      {
         setting.invert[ii] = 0;
      }
      if (setting.termination[ii] !=  oldTermination[ii])
      {
        PinMode mode =  INPUT;
        switch (setting.termination[ii]) 
        {
          case 0:
            mode =  INPUT;
            break;
          case 1:
            mode =  INPUT_PULLDOWN;
            break;
          case 2:
            mode =  INPUT_PULLUP;
            break;
          default:
            mode =  INPUT;
            break;
        }
        pinMode(switchPin[ii], mode);
        oldTermination[ii] = setting.termination[ii];      
      }
      if (setting.interlockOn[ii] > 0)
      {
        setting.interlockOn[ii] = 1;
      }
      else
      {
         setting.interlockOn[ii] = 0;
      }
      if (setting.interlockOn[ii] !=  oldInterlockOn[ii])
      {
        if (setting.interlockOn[ii] == 0)  digitalWrite(powerPin[ii], LOW);
        if (setting.interlockOn[ii] == 1)  digitalWrite(powerPin[ii], HIGH);
        oldInterlockOn[ii] = setting.interlockOn[ii];      
      }
    }
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
