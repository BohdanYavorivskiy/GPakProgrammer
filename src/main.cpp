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

#define I2C_SLAVE_ADDR 0x68
#define I2C_BUFFER_SIZE 512

#define SUCCESS 0
#define ERROR 1

#define ERROR_FILE_NOT_OPEN 11
#define ERROR_CRC_FAIL 12
#define ERROR_FILE_SIZE 13

/////////////////////////////////////////////////////
// DECLARATIONS

void showAllStatusesTft();

void runTestProcedure();
void resetRestStates();

uint8_t setI2CSlaveSelection(uint8_t devI);
String getI2cErrDetails(uint8_t err);

void setCriticalErrorState();

void click3();

/////////////////////////////////////////////////////
// SD CARD

#define SD_CS 33
#define SD_SCLK 25
#define SD_MISO 27
#define SD_MOSI 26

SPIClass SDSPI(VSPI);

#define DESIGN_SIZE 256
uint8_t testStatus = SUCCESS;
String testStatusStr = "OK";

uint8_t readDesign(uint8_t *buf)
{
  File binFile = SD.open("/fw.bin", FILE_READ);
  if (!binFile)
    return ERROR_FILE_NOT_OPEN;

  int i = 0;
  uint32_t crc = 0;

  uint8_t err = SUCCESS;

  if (binFile.available() >= DESIGN_SIZE + 1)
  {
    binFile.read(buf, 256);
    for (int j = 0; j < DESIGN_SIZE; ++j)
    {
      crc += buf[j];
      Serial.print(String(buf[i], 16) + " ");
    }

    const uint8_t expCrc = binFile.read();
    Serial.println("expCrc " + String(expCrc) + ";   crc " + String(crc & 0xFF));

    if ((crc & 0xFF) == expCrc)
      err = SUCCESS;
    else
      err = ERROR_CRC_FAIL;
  }
  else
  {
    err = ERROR_FILE_SIZE;
  }

  binFile.close();
  return err;
}

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
  click3();
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
  {
    runTestProcedure();
    if (testStatus != SUCCESS)
    {
      setCriticalErrorState();

      tft.drawString("Err: \"" + testStatusStr + "\"                          ", 3, 20, 2);
    }
  }
  else
  {
    resetRestStates();
  }
  readyForTest = !readyForTest;
}

void longPressDuring3()
{
  Serial.println("longPressDuring3");
}

/////////////////////////////////////////////////////
// IO Expanders, LEDs
String getI2cErrDetails(uint8_t err)
{
  switch (err)
  {
  case 0:
    return "Success";
  case 1:
    return "Data too long to fit in transmit buffer";
  case 2:
    return "Received NACK on transmit of address";
  case 3:
    return "Received NACK on transmit of data";
  case 4:
    return "Other error";
  default:
    return "Unknown error";
  }
}

PCF8574 IoExp1_4(0x38);
PCF8574 IoExp5_8(0x39);
PCF8574 IoExp9_12(0x3a);
PCF8574 IoExp12_16(0x3b);
PCF8574 IoExp17_20(0x3c);
PCF8574 IoExp21_24(0x3d);

uint8_t setAllIoState(bool state)
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

  if (IoExp1_4.lastError() != SUCCESS)
    return ERROR;
  if (IoExp5_8.lastError() != SUCCESS)
    return ERROR;
  if (IoExp9_12.lastError() != SUCCESS)
    return ERROR;
  if (IoExp12_16.lastError() != SUCCESS)
    return ERROR;
  if (IoExp17_20.lastError() != SUCCESS)
    return ERROR;
  if (IoExp21_24.lastError() != SUCCESS)
    return ERROR;

  return SUCCESS;
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

uint8_t clearAllLeds()
{
  return setAllIoState(false);
}

uint8_t setLedStateFotI2cDevice(uint8_t devId, uint8_t st)
{
  PCF8574 *worker = getIoExpForDev(devId);
  if (worker == nullptr)
    return ERROR;

  if (st == SUCCESS)
    worker->write(((devId % 4) * 2) + 1, HIGH);
  else
    worker->write(((devId % 4) * 2), HIGH);

  return worker->lastError();
}

/////////////////////////////////////////////////////
/// I2C MUXes
#define TCA9548A_DDR 0x70 // Default I2C address of TCA9548A


uint8_t TCA9548ASaForDevId(uint8_t devId)
{
  if (devId < 8)
    return TCA9548A_DDR;
  if (devId < 16)
    return TCA9548A_DDR | ((uint8_t)0x04);
  if (devId < 24)
    return TCA9548A_DDR | ((uint8_t)0x02);

  return 0;
}

uint8_t TCA9548AConfForDevId(uint8_t devId)
{
  return (1 << (((uint8_t)(7 - (devId % 8))) & 0x07));
}

uint8_t setI2CSlaveSelection(uint8_t devI, bool state)
{
  Wire.beginTransmission(TCA9548ASaForDevId(devI));
  const uint8_t conf = state ? TCA9548AConfForDevId(devI) : 0;
  Wire.write(conf);
  return Wire.endTransmission();
}

uint8_t disableI2cSlaveSelectionsForIc(uint8_t IcI2CSa)
{
  Wire.beginTransmission(IcI2CSa);
  Wire.write(0);
  return Wire.endTransmission();
}

uint8_t disableI2cSlaveSelectionsForIcForce(uint8_t IcI2CSa)
{
  uint8_t err = SUCCESS;
  for (uint8_t i = 0; i < 5; ++i)
  {
    delay(10);
    err = disableI2cSlaveSelectionsForIc(IcI2CSa);

    Serial.println("I2C MUX termination tries " + String(i) + "   I2C SA" + String(IcI2CSa, 16) + "  Err ->" + getI2cErrDetails(err));

    if (err == SUCCESS)
      return err;
  }
  return err;
}

uint8_t disableAllI2CSlaveSelections()
{
  uint8_t err = SUCCESS;

  err = disableI2cSlaveSelectionsForIcForce(TCA9548A_DDR | ((uint8_t)0x00));
  if (err != SUCCESS)
    return err;

  err = disableI2cSlaveSelectionsForIcForce(TCA9548A_DDR | ((uint8_t)0x04));
  if (err != SUCCESS)
    return err;

  err = disableI2cSlaveSelectionsForIcForce(TCA9548A_DDR | ((uint8_t)0x02));
  return err;
}

////////////////////////////////////////////////////////
/// GreenPAKs

uint8_t GpakDeviceStatuses[24];

uint8_t devX(uint8_t devIndex) {
  uint8_t rightLedsMove = 0;
  if ((devIndex % 8) >= 4)
    rightLedsMove = 25;

  return 20 + ((devIndex % 8) * 25) + rightLedsMove; }

uint8_t devY(uint8_t devIndex)
{
  uint8_t row = (devIndex / 8);

  return SCR_LOG_HEIGTH + 10 + (row * 20);
}

void showAllStatusesTft()
{
  for (uint8_t dev = 0; dev < sizeof(GpakDeviceStatuses); dev++)
  {
    uint16_t colour = GpakDeviceStatuses[dev] == SUCCESS ? TFT_GREEN : TFT_RED;
    Serial.println(String(dev) + " " + String(GpakDeviceStatuses[dev]));

    // tft.drawString(String(dev) + ":", devX(dev), devY(dev), 1);
    tft.fillEllipse(devX(dev), devY(dev), 3, 3, colour);
  }
}

void showAllStatusesLed()
{
  for (uint8_t dev = 0; dev < sizeof(GpakDeviceStatuses); dev++)
    setLedStateFotI2cDevice(dev, (GpakDeviceStatuses[dev] == SUCCESS ? I2cDevState::Success : I2cDevState::Error));
}

void setCriticalErrorState()
{
  clearFullScreen();
  cleadLogSection();

  tft.drawString("CRITICAL ERROR, RESET POWER", 3, 3, 2);

  for (uint8_t i = 0; i < sizeof(GpakDeviceStatuses); ++i)
    setLedStateFotI2cDevice(i, ERROR);
}

void writeDataToAllDev(uint8_t memAddr, const uint8_t *data, size_t quantity, uint8_t displayLed, uint8_t isSecondConf)
{
  for (uint8_t chipI = 0; chipI < sizeof(GpakDeviceStatuses); ++chipI)
  {
    if (isSecondConf)
      if (GpakDeviceStatuses[chipI] != SUCCESS)
        continue;

    // Select chip
    testStatus = setI2CSlaveSelection(chipI, true);
    if (testStatus != SUCCESS)
    {
      Serial.println("ERR MUX enable error for " + String(chipI) + "  Err ->" + getI2cErrDetails(testStatus));
      testStatusStr = "I2C mux conf for " + String(chipI);
      return;
    }
    delay(2);

    // write NVM
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    size_t transferedBytes = Wire.write(&memAddr, 1);
    transferedBytes = Wire.write(data, quantity);
    const uint8_t err = Wire.endTransmission();
    Serial.println("I2C dev " + String(chipI) + +"  Err ->" + getI2cErrDetails(err) + "  Tx bytes " + String(transferedBytes));

    uint8_t deviceStatus = err;
    if (transferedBytes != quantity)
      deviceStatus = ERROR;

    GpakDeviceStatuses[chipI] = deviceStatus;

    delay(2);

    if (displayLed)
    {
      testStatus = setLedStateFotI2cDevice(chipI, GpakDeviceStatuses[chipI]);
      if (testStatus != SUCCESS)
      {
        Serial.println("ERR set LED error for " + String(chipI) + "  Err ->" + getI2cErrDetails(testStatus));
        testStatusStr = "LED conf for " + String(chipI);
        return;
      }
    }
    testStatus = setI2CSlaveSelection(chipI, false);
    if (testStatus != 0)
    {
      Serial.println("ERR MUX disable error for " + String(chipI) + "  Err ->" + getI2cErrDetails(testStatus));
      testStatusStr = "I2C mux reset for " + String(chipI);
      return;
    }
  }
}

void resetRestStates()
{
  clearFullScreen();
  cleadLogSection();
  if (disableAllI2CSlaveSelections() != SUCCESS)
  {
    setCriticalErrorState();
    return;
  }

  uint8_t err = SUCCESS;
  for (uint8_t i = 0; i < 5; ++i)
  {
    err = setAllIoState(true);
    if (err != SUCCESS)
      break;
    delay(30);
    err = setAllIoState(false);
    if (err != SUCCESS)
      break;
    delay(30);
  }
  if (err != SUCCESS)
  {
    setCriticalErrorState();
    return;
  }

  tft.drawString("Press TEST to start", 3, 3, 2);
}

void runTestProcedure()
{
  // cleas screen
  clearFullScreen();
  cleadLogSection();

  // reset test terult statuses
  for (uint8_t chipI = 0; chipI < sizeof(GpakDeviceStatuses); ++chipI)
    GpakDeviceStatuses[chipI] = ERROR;

  // read file
  uint8_t DesignPN[DESIGN_SIZE] = {0};
  testStatus = readDesign(DesignPN);

  String statusStr = "File error - FAIL";
  if (testStatus == SUCCESS)
    statusStr = "File read, CRC check - PASS";
  if (testStatus == ERROR_CRC_FAIL)
    statusStr = "CRC check - FAIL";
  if (testStatus == ERROR_FILE_NOT_OPEN)
    statusStr = "Open file - FAIL";
  if (testStatus == ERROR_FILE_SIZE)
    statusStr = "File is too small - FAIL";
  tft.drawString(statusStr, 3, 3, 2);

  if (testStatus != SUCCESS)
  {
    Serial.println("ERR FILE error");
    testStatusStr = statusStr;
    return;
  }

  const uint8_t StartMemAddr = 0x00;
  const uint8_t ForceToEnableByteIndex = 0x72;
  const uint8_t ForceToEnableByte = DesignPN[ForceToEnableByteIndex];
  DesignPN[ForceToEnableByteIndex] = 0;

  tft.drawString("Programming...", 3, 20, 2);

  // reset LEDs
  testStatus = clearAllLeds();
  if (testStatus != SUCCESS)
  {
    Serial.println("ERR reset LEDs error");
    testStatus = ERROR;
    testStatusStr = "LED preconfiguretion ";
    return;
  }
  delay(10);

  writeDataToAllDev(0, DesignPN, DESIGN_SIZE, false, false);
  if (testStatus != SUCCESS)
  {
    Serial.println("ERR writeDataToAllDev DesignPN error");
    return;
  }

  delay(2000);

  writeDataToAllDev(ForceToEnableByteIndex, &ForceToEnableByte, 1, true, true);
  if (testStatus != SUCCESS)
  {
    Serial.println("ERR writeDataToAllDev ForceToEnableByte error");
    return;
  }

  showAllStatusesTft();
  showAllStatusesLed();

  tft.drawString("Programming FINISHED", 3, 20, 2);
}

/////////////////////////////////////////////////////
/// SETUP
void setup()
{
  Serial.begin(115200);
  Serial.println("setup---");

  Wire.begin();
  Wire.setBufferSize(I2C_BUFFER_SIZE);

  tft.init();
  tft.fontHeight(2);
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);

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
  Serial.println("setup finished");
}

void loop()
{
  button1.tick();
  button2.tick();
  button3.tick();
}
