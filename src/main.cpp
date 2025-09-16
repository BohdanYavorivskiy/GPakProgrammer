#include "OneButton.h"
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <HardwareSerial.h>

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

void testAll();
void resetAll();

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

void click1() { testAll(); }

void longPressDuring1() { resetAll(); }

void click2() { testAll(); }

void longPressDuring2() { resetAll(); }

/////////////////////////////////////////////////////

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

void drawAnalogIndications(int indicatorIndex, uint16_t colorMin, int mVmin, uint16_t colorMax, int mVmax)
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

int testPin(int pinIn, int pinInA, int pinOut)
{
  int minmV = 9999;
  int maxmV = -9999;
  int errorCode = SUCCESS;

  setDefaultPinmode();
  pinMode(pinOut, OUTPUT);

  for (int i = 0; i < 10; ++i)
  {
    if (errorCode != SUCCESS)
      break;

    drawIndicator(pinIn, i % 2 != 0 ? TFT_WHITE : TFT_BLACK);
    delay(10);
    for (int x = 0; x < 50; ++x)
    {
      const int testableState = x % 2;
      digitalWrite(pinOut, testableState);
      delay(1);
      if (testableState != digitalRead(pinIn) || chackAllExclude(pinIn, HIGH) != SUCCESS)
      {
        errorCode = ERROR;
        break;
      }
      else
      {
        const int mv = analogReadMilliVolts(pinInA);
        minmV = minmV > mv ? mv : minmV;
        maxmV = maxmV < mv ? mv : maxmV;
      }
    }
  }

  drawIndicator(pinIn, errorCode == SUCCESS ? TFT_GREEN : TFT_RED);


  int percentMinmV = (minmV < 0 || minmV == 9999) ? 0 : minmV / 33; // mv / 3300 mV * 100%
  int percentMaxmV = maxmV < 0 ? 0 : maxmV / 33;

  uint16_t colorMinmV = TFT_GREEN;
  if (percentMinmV > 30)
    colorMinmV = TFT_YELLOW;
  if (percentMinmV > 70)
    colorMinmV = TFT_RED;

  uint16_t colorMaxV = TFT_GREEN;
  if (percentMaxmV < 70)
    colorMaxV = TFT_YELLOW;
  if (percentMaxmV < 30)
    colorMaxV = TFT_RED;

  drawAnalogIndications(pinIn, colorMinmV, percentMinmV, colorMaxV, percentMaxmV);
  return errorCode;
}

void testAll()
{
  testPin(PIN1_IN, PIN1_INA, PIN1_OUT);
  testPin(PIN2_IN, PIN2_INA, PIN2_OUT);
  testPin(PIN3_IN, PIN3_INA, PIN3_OUT);
  testPin(PIN4_IN, PIN4_INA, PIN4_OUT);
}

void resetVoltages()
{
  tft.fillRect(80, 7, 200, 15, TFT_BLACK);
  tft.fillRect(80, tft.height() * 0.25 + 7, 200, 15, TFT_BLACK);
  tft.fillRect(80, tft.height() * 0.5 + 7, 200, 15, TFT_BLACK);
  tft.fillRect(80, tft.height() * 0.75 + 7, 200, 15, TFT_BLACK);
}

void resetAll()
{
  drawIndicator(PIN1_IN, TFT_BLACK);
  drawIndicator(PIN2_IN, TFT_BLACK);
  drawIndicator(PIN3_IN, TFT_BLACK);
  drawIndicator(PIN4_IN, TFT_BLACK);
  resetVoltages();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("1231234567890-");
  tft.init();
  tft.fontHeight(2);
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("P1", 10, 3, 4);
  tft.drawString("P2", 10, tft.height() * 0.25 + 3, 4);
  tft.drawString("P3", 10, tft.height() * 0.5 + 3, 4);
  tft.drawString("P4", 10, tft.height() * 0.75 + 3, 4);

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
  setDefaultPinmode();
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
