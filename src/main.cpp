#include "OneButton.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>

/////////////////////////////////////////////////////
#define PIN1_OUT 21
#define PIN2_OUT 22
#define PIN3_OUT 17
#define PIN4_OUT 2

#define PIN1_IN 36
#define PIN2_IN 37
#define PIN3_IN 38
#define PIN4_IN 39

#define PIN1_INA 32
#define PIN2_INA 33
#define PIN3_INA 25
#define PIN4_INA 26

#define ERROR -1
#define SUCCESS 0

/////////////////////////////////////////////////////
// DECLARATIONS

void i2cBusCheckConnection();
void showI2CDeviceStatuses();

void showAllStatuses();

/////////////////////////////////////////////////////
// TFT

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

/////////////////////////////////////////////////////
// Buttons
#define Btn1Io 35
#define Btn2Io 0

OneButton button1(Btn1Io, true);
OneButton button2(Btn2Io, true);

void click1()
{
  tft.fillEllipse(60, 13, 10, 10, TFT_BLUE);
  i2cBusCheckConnection();
  showI2CDeviceStatuses();
}

void longPressDuring1()
{
  tft.fillEllipse(60, 13, 10, 10, TFT_GREEN);
  showAllStatuses();
}

void click2() { tft.fillEllipse(60, 13, 10, 10, TFT_CYAN); }

void longPressDuring2() { tft.fillEllipse(60, 13, 10, 10, TFT_MAGENTA); }

/////////////////////////////////////////////////////

uint8_t I2CDeviceStatuses[128];

void i2cBusCheckConnection()
{
  uint8_t error, address;
  int count = 0;

  for (address = 0; address < 128; address++)
  {
    Wire.beginTransmission(address);
    I2CDeviceStatuses[address] = Wire.endTransmission();
  }
}

void showI2CDeviceStatuses()
{

  String foundI2CDev;
  for (uint8_t address = 0; address < 128; address++)
  {
    if (I2CDeviceStatuses[address] == 0)
    {
      if (!foundI2CDev.isEmpty())
        foundI2CDev += ", ";

      foundI2CDev += String(address);
    }
  }

  tft.drawString(foundI2CDev, 80, 3, 1);
}

/////////////////////////////////////////////////////
uint8_t GpakDeviceStatuses[24];

uint8_t devX(uint8_t devIndex)
{
  if (devIndex < 6)
    return 10;

  else if (devIndex < 12)
    return 60;
  else if (devIndex < 18)
    return 110;
  else
    return 170;
}

uint8_t devY(uint8_t devIndex)
{
  uint8_t row = devIndex - ((devIndex / 6) * 6);

  return 75 + (row * 10);
}

void showAllStatuses()
{
  for (uint8_t dev = 0; dev < sizeof(GpakDeviceStatuses); dev++)
  {
    uint16_t colour = TFT_GREEN;
    if (dev % 3 == 0)
      colour = TFT_RED;

    tft.drawString(String(dev) + ":", devX(dev), devY(dev), 1);
    tft.fillEllipse(devX(dev) + 25, devY(dev) + 3, 3, 3, colour);
  }
}

/////////////////////////////////////////////////////

void setup()
{
  Serial.begin(9600);
  Serial.println("1231234567890-");

  Wire.begin();

  tft.init();
  tft.fontHeight(2);
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  // tft.drawString("P1", 10, 3, 4);
  // tft.drawString("P2", 10, tft.height() * 0.25 + 3, 4);
  // tft.drawString("P3", 10, tft.height() * 0.5 + 3, 4);
  // tft.drawString("P4", 10, tft.height() * 0.75 + 3, 4);

  // img.setFreeFont(&FreeSansBold18pt7b);

  // ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  // ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  // ledcWrite(pwmLedChannelTFT,100);

  /////////////////////////////////////////////////////

  button1.attachClick(click1);
  button1.attachDuringLongPress(longPressDuring1);

  // link the button 2 functions.
  button2.attachClick(click2);
  button2.attachDuringLongPress(longPressDuring2);

  /////////////////////////////////////////////////////
}

void loop()
{
  if (Serial.available())
  {
    Serial.println(Serial.read());
    Serial.println("123");
  }
  // testPin(PIN1_IN, PIN1_OUT);
  // testPin(PIN2_IN, PIN2_OUT);
  // testPin(PIN3_IN, PIN3_OUT);
  // testPin(PIN4_IN, PIN4_OUT);

  button1.tick();
  button2.tick();
}

/*

void drawIndicator(int indicatorIndex, uint16_t color)
{
  if (indicatorIndex == PIN1_IN)
    tft.fillEllipse(60, 13, 10, 10, color);
  else if (indicatorIndex == PIN2_IN)
    tft.fillEllipse(60, tft.height() * 0.25 + 13, 10, 10, color);
  else if (indicatorIndex == PIN3_IN)
    tft.fillEllipse(60, tft.height() * 0.5 + 13, 10, 10, color);
  else if (indicatorIndex == PIN4_IN)
    tft.fillEllipse(60, tft.height() * 0.75 + 13, 10, 10, color);
}

void drawAnalogIndications(int indicatorIndex, uint16_t colorMin, int mVmin,
uint16_t colorMax, int mVmax)
{
  const uint8_t fontIndex = 2;
  tft.setTextColor(colorMin, TFT_BLACK);

  if (indicatorIndex == PIN1_IN)
  {
    tft.fillRect(80, 7, 200, 15, TFT_BLACK);
    tft.drawNumber(mVmin, 80, 7, fontIndex);
  }
  else if (indicatorIndex == PIN2_IN)
  {
    tft.fillRect(80, tft.height() * 0.25 + 7, 200, 15, TFT_BLACK);
    tft.drawNumber(mVmin, 80, tft.height() * 0.25 + 7, fontIndex);
  }
  else if (indicatorIndex == PIN3_IN)
  {
    tft.fillRect(80, tft.height() * 0.5 + 7, 200, 15, TFT_BLACK);
    tft.drawNumber(mVmin, 80, tft.height() * 0.5 + 7, fontIndex);
  }
  else if (indicatorIndex == PIN4_IN)
  {
    tft.fillRect(80, tft.height() * 0.75 + 7, 200, 15, TFT_BLACK);
    tft.drawNumber(mVmin, 80, tft.height() * 0.75 + 7, fontIndex);
  }

  tft.setTextColor(colorMax, TFT_BLACK);
  if (indicatorIndex == PIN1_IN)
  {
    tft.drawNumber(mVmax, 120, 7, fontIndex);
  }
  else if (indicatorIndex == PIN2_IN)
  {
    tft.drawNumber(mVmax, 120, tft.height() * 0.25 + 7, fontIndex);
  }
  else if (indicatorIndex == PIN3_IN)
  {
    tft.drawNumber(mVmax, 120, tft.height() * 0.5 + 7, fontIndex);
  }
  else if (indicatorIndex == PIN4_IN)
  {
    tft.drawNumber(mVmax, 120, tft.height() * 0.75 + 7, fontIndex);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (indicatorIndex == PIN1_IN)
  {
    tft.drawString("%   min max", 155, 7, fontIndex);
  }
  else if (indicatorIndex == PIN2_IN)
  {
    tft.drawString("%   min max", 155, tft.height() * 0.25 + 7, fontIndex);
  }
  else if (indicatorIndex == PIN3_IN)
  {
    tft.drawString("%   min max", 155, tft.height() * 0.5 + 7, fontIndex);
  }
  else if (indicatorIndex == PIN4_IN)
  {
    tft.drawString("%   min max", 155, tft.height() * 0.75 + 7, fontIndex);
  }
}

void setDefaultPinmode()
{
  pinMode(PIN1_OUT, INPUT);
  pinMode(PIN2_OUT, INPUT);
  pinMode(PIN3_OUT, INPUT);
  pinMode(PIN4_OUT, INPUT);

  digitalWrite(PIN1_OUT, LOW);
  digitalWrite(PIN2_OUT, LOW);
  digitalWrite(PIN3_OUT, LOW);
  digitalWrite(PIN4_OUT, LOW);

  pinMode(PIN1_IN, INPUT_PULLUP);
  pinMode(PIN2_IN, INPUT_PULLUP);
  pinMode(PIN3_IN, INPUT_PULLUP);
  pinMode(PIN4_IN, INPUT_PULLUP);

  pinMode(PIN1_INA, ANALOG);
  pinMode(PIN2_INA, ANALOG);
  pinMode(PIN3_INA, ANALOG);
  pinMode(PIN4_INA, ANALOG);
}

int chackAllExclude(int excludePinIn, uint8_t state)
{
  if (excludePinIn != PIN1_IN && digitalRead(PIN1_IN) != state)
    return ERROR;
  if (excludePinIn != PIN2_IN && digitalRead(PIN2_IN) != state)
    return ERROR;
  if (excludePinIn != PIN3_IN && digitalRead(PIN3_IN) != state)
    return ERROR;
  if (excludePinIn != PIN4_IN && digitalRead(PIN4_IN) != state)
    return ERROR;
  return SUCCESS;
}



*/