#include "OneButton.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <PCF8574.h>

#define SD_DEBUG 1

#include <FS.h>
#include <SD.h>
#include <SPI.h>

/////////////////////////////////////////////////////
// #define RES_1 13
// #define RES_2 32
// #define RES_3 17
// #define A_0 2
// #define A_1 15
// #define A_2 12

#define I2C_SLAVE_ADDR 0x68

#define SUCCESS 0
#define ERROR 1

#define ERROR_FILE_NOT_OPEN 11
#define ERROR_CRC_FAIL 12

/////////////////////////////////////////////////////
// SD CARD

#define SD_CS 33
#define SD_SCLK 25
#define SD_MISO 27
#define SD_MOSI 26

SPIClass SDSPI(HSPI);

uint8_t Design[256];

uint8_t readFile()
{

  File binFile = SD.open("/fw.bin", FILE_READ);
  if (!binFile)
    return ERROR_FILE_NOT_OPEN;

  int i = 0;
  uint32_t crc = 0;

  uint8_t err = SUCCESS;

  while (binFile.available())
  {
    Design[i] = binFile.read();
    Serial.print(String(Design[i], 16) + " ");
    crc += Design[i];
    ++i;

    if (i == sizeof(Design))
    {
      if (binFile.available())
      {
        const uint8_t expCrc = binFile.read();
        Serial.println("expCrc " + String(expCrc) + ";   crc " + String(crc & 0xFF));

        if ((crc & 0xFF) == expCrc)
          err = SUCCESS;
        else
          err = ERROR_CRC_FAIL;

        break;
      }
    }
  }

  binFile.close();

  return err;
}

/////////////////////////////////////////////////////
// DECLARATIONS

void i2cBusCheckConnection();
void showI2CDeviceStatuses();

void showAllStatusesTft();

void runTestProcedure();
void resetRestStates();

/////////////////////////////////////////////////////
// TFT

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

#define SCR_LOG_HEIGTH 70
#define SCR_WIDTH TFT_HEIGHT

void clearFullScreen()
{
  tft.fillScreen(TFT_BLACK);
}

void cleadLogSection()
{
  tft.fillRect(0, 0, TFT_WIDTH, SCR_LOG_HEIGTH, TFT_BLACK);
  tft.drawLine(0, SCR_LOG_HEIGTH, SCR_WIDTH, SCR_LOG_HEIGTH, TFT_WHITE);
}

/////////////////////////////////////////////////////
// Buttons
#define Btn1Io 35
#define Btn2Io 0
#define Btn3Io 36

OneButton button1(Btn1Io, true);
OneButton button2(Btn2Io, true);
OneButton button3(Btn3Io, true);

void click1()
{
  Serial.println("click1");
  runTestProcedure();
}

void longPressDuring1()
{
  Serial.println("longPressDuring1");
  tft.fillEllipse(60, 13, 10, 10, TFT_GREEN);
}

void click2()
{
  Serial.println("click2");
  showAllStatusesTft();
}

void longPressDuring2()
{
  Serial.println("longPressDuring2");
  tft.fillEllipse(60, 13, 10, 10, TFT_MAGENTA);
}

void click3()
{
  Serial.println("click3");
  static bool readyForTest = true;
  if (readyForTest)
    runTestProcedure();
  else
    resetRestStates();

  readyForTest = !readyForTest;
}

void longPressDuring3()
{
  Serial.println("longPressDuring3");
}

/////////////////////////////////////////////////////
// IO Expanders

PCF8574 IoExp1_4(0x38);
PCF8574 IoExp5_8(0x39);
PCF8574 IoExp9_12(0x3a);
PCF8574 IoExp12_16(0x3b);
PCF8574 IoExp17_20(0x3c);
PCF8574 IoExp21_24(0x3d);

void setAllIoState(bool state)
{
  if (state)
  {
    IoExp1_4.selectAll();
    IoExp5_8.selectAll();
    IoExp9_12.selectAll();
    IoExp12_16.selectAll();
    IoExp17_20.selectAll();
    IoExp21_24.selectAll();
  }
  else
  {
    IoExp1_4.selectNone();
    IoExp5_8.selectNone();
    IoExp9_12.selectNone();
    IoExp12_16.selectNone();
    IoExp17_20.selectNone();
    IoExp21_24.selectNone();
  }
}

PCF8574 *getIoExpForDev(uint8_t devId)
{
  if (devId < 4)
    return &IoExp1_4;
  else if (devId < 8)
    return &IoExp5_8;
  else if (devId < 12)
    return &IoExp9_12;
  else if (devId < 16)
    return &IoExp12_16;
  else if (devId < 20)
    return &IoExp17_20;
  else if (devId < 24)
    return &IoExp21_24;

  return nullptr;
}

enum I2cDevState
{
  Success,
  Error
};

void clearAllLeds()
{
  setAllIoState(false);
}

void setLedStateFotI2cDevice(uint8_t devId, I2cDevState st)
{
  if (st == Success)
    getIoExpForDev(devId)->write(((devId % 4) * 2), HIGH);
  else
    getIoExpForDev(devId)->write(((devId % 4) * 2) + 1, HIGH);
}

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

  // tft.drawString(foundI2CDev, 80, 3, 1);
}

/////////////////////////////////////////////////////

#define TCA9548A_DDR 0x70 // Default I2C address of TCA9548A
// devI
//  x   x  |  x   x   x | x   x   x  |
//         | a2  a1  a0 | I2C on mux |
void selectI2cSlave(uint8_t devI)
{

  Wire.beginTransmission(TCA9548A_DDR | ((uint8_t)(((uint8_t)(devI >> 3)) & 0x07)));
  Wire.write(1 << (devI & 0x07));
  Wire.endTransmission();
}

uint8_t GpakDeviceStatuses[24];

uint8_t devX(uint8_t devIndex) { return 10 + (devIndex / 6) * 60; }

uint8_t devY(uint8_t devIndex)
{
  uint8_t row = devIndex - ((devIndex / 6) * 6);

  return SCR_LOG_HEIGTH + 5 + (row * 10);
}

void showAllStatusesTft()
{
  for (uint8_t dev = 0; dev < sizeof(GpakDeviceStatuses); dev++)
  {
    uint16_t colour = GpakDeviceStatuses[dev] == SUCCESS ? TFT_GREEN : TFT_RED;

    tft.drawString(String(dev) + ":", devX(dev), devY(dev), 1);
    tft.fillEllipse(devX(dev) + 25, devY(dev) + 3, 3, 3, colour);
  }
}

void showAllStatusesLed()
{
  for (uint8_t dev = 0; dev < sizeof(GpakDeviceStatuses); dev++)
    setLedStateFotI2cDevice(dev, (GpakDeviceStatuses[dev] == SUCCESS ? I2cDevState::Success : I2cDevState::Error));
}

void resetRestStates()
{
  clearFullScreen();
  cleadLogSection();
  for (uint8_t i = 0; i < 5; ++i)
  {
    setAllIoState(true);
    delay(150);
    setAllIoState(false);
    delay(150);
  }

  tft.drawString("Press TEST to start", 3, 3, 2);
}

void runTestProcedure()
{
  clearFullScreen();
  cleadLogSection();
  clearAllLeds();

  // read file
  uint8_t err = readFile();
  String statusStr;
  if (err == SUCCESS)
    statusStr = "File read, CRC check - PASS";
  else if (err == ERROR_CRC_FAIL)
    statusStr = "CRC check - FAIL";
  else if (err == ERROR_FILE_NOT_OPEN)
    statusStr = "Open file - FAIL";
  else
    statusStr = "File error - FAIL";

  tft.drawString(statusStr, 3, 3, 1);

  if (err != SUCCESS)
  {
    Serial.println("FILE error");
    return;
  }

  Serial.println("Start writting");

  delay(100);
  for (uint8_t chipI = 0; chipI < sizeof(GpakDeviceStatuses); ++chipI)
  {
    // Select chip
    selectI2cSlave(chipI);
    delay(5);

    // write NVM
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(Design, sizeof(Design));
    err = Wire.endTransmission();
    Serial.println("I2C dev " + String(chipI) + "   res " + String(err));

    // save res for display
    GpakDeviceStatuses[chipI] = err;

    delay(10);

    // set led state
  }

  showAllStatusesTft();
  showAllStatusesLed();
}

/////////////////////////////////////////////////////

void setup()
{
  Serial.begin(9600);
  Serial.println("setup---");

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

  button3.attachClick(click3);
  button3.attachDuringLongPress(longPressDuring3);

  /////////////////////////////////////////////////////

  SDSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SDSPI))
  {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("SD Card Mount Successful");

  //////////////////////////////////////////////////////

  IoExp1_4.begin();
  IoExp5_8.begin();
  IoExp9_12.begin();
  IoExp12_16.begin();
  IoExp17_20.begin();
  IoExp21_24.begin();

  resetRestStates();
  //////////////////////////////////////////////////////
}

void loop()
{
  if (Serial.available())
  {
    Serial.println(Serial.read());
    Serial.println("123");
  }

  button1.tick();
  button2.tick();
  button3.tick();
}
