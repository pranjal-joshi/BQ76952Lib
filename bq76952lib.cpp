/*
* Description :   Source file of BQ76952 BMS IC (by Texas Instruments) for Arduino platform.
* Author      :   Pranjal Joshi
* Date        :   17/10/2020 
* License     :   MIT
* This code is published as open source software. Feel free to share/modify.
*/

#include "bq76952lib.h"
#include <Wire.h>

#if (defined(AVR))
	#include <avr\pgmspace.h>
#else
	#include <pgmspace.h>
#endif

// Library config
#define DBG_BAUD            115200
#define BQ_I2C_ADDR_WRITE   0x10
#define BQ_I2C_ADDR_READ    0x11
bool BQ_DEBUG = false;

// BQ76952 - Address Map
#define CMD_DIR__SUBCMD_LOW            0x3E
#define CMD_DIR__SUBCMD_HI             0x3F
#define CMD_DIR__RESP_LEN              0x61
#define CMD_DIR__RESP_START            0x40
#define CMD_DIR__RESP_CHKSUM           0x60

// BQ76952 - Voltage measurement commands
#define CMD_READ_VOLTAGE_CELL_1   0x14
#define CMD_READ_VOLTAGE_CELL_2   0x16
#define CMD_READ_VOLTAGE_CELL_3   0x18
#define CMD_READ_VOLTAGE_CELL_4   0x1A
#define CMD_READ_VOLTAGE_CELL_5   0x1C
#define CMD_READ_VOLTAGE_CELL_6   0x1E
#define CMD_READ_VOLTAGE_CELL_7   0x20
#define CMD_READ_VOLTAGE_CELL_8   0x22
#define CMD_READ_VOLTAGE_CELL_9   0x24
#define CMD_READ_VOLTAGE_CELL_10  0x26
#define CMD_READ_VOLTAGE_CELL_11  0x28
#define CMD_READ_VOLTAGE_CELL_12  0x2A
#define CMD_READ_VOLTAGE_CELL_13  0x2C
#define CMD_READ_VOLTAGE_CELL_14  0x2E
#define CMD_READ_VOLTAGE_CELL_15  0x30
#define CMD_READ_VOLTAGE_CELL_16  0x32
#define CMD_READ_VOLTAGE_STACK    0x34
#define CMD_READ_VOLTAGE_PACK     0x36

// BQ76952 - Direct Commands
#define CMD_DIR_SPROTEC           0x02
#define CMD_DIR_FPROTEC           0x03
#define CMD_DIR_SFET              0x06
#define CMD_DIR_FFET              0x07
#define CMD_DIR_VCELL_1           0x14
#define CMD_DIR_INT_TEMP          0x68
#define CMD_DIR_CC2_CUR           0x3A

// Alert Bits in BQ76952 registers
#define BIT_SA_SC_DCHG            7
#define BIT_SA_OC2_DCHG           6
#define BIT_SA_OC1_DCHG           5
#define BIT_SA_OC_CHG             4
#define BIT_SA_CELL_OV            3
#define BIT_SA_CELL_UV            2

// Inline functions
#define CELL_NO_TO_ADDR(cellNo) (0x14 + ((cellNo-1)*2))

//// LOW LEVEL FUNCTIONS ////

void bq76952::initBQ(void) {
  Wire.begin();
}

// Send Direct command
unsigned int bq76952::directCommand(byte command) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(command);
  Wire.endTransmission();

  Wire.requestFrom(BQ_I2C_ADDR_READ, 2);
  while(!Wire.available());
  byte lsb = Wire.read();
  byte msb = Wire.read();

  debugPrint(F("[+] Direct Cmd SENT -> "));
  debugPrintlnCmd((uint16_t)command);
  debugPrint(F("[+] Direct Cmd RESP <- "));
  debugPrintlnCmd((uint16_t)(msb << 8 | lsb));

  return (unsigned int)(msb << 8 | lsb);
}

// Send Sub-command
void bq76952::subCommand(unsigned int data) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(CMD_DIR__SUBCMD_LOW);
  Wire.write((byte)data & 0x00FF);
  Wire.write((byte)(data >> 8) & 0x00FF);
  Wire.endTransmission();

  debugPrint(F("[+] Sub Cmd SENT to 0x3E -> "));
  debugPrintlnCmd((uint16_t)data);
}

// Read subcommand response
unsigned int bq76952::subCommandResponseInt(void) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(CMD_DIR__RESP_START);
  Wire.endTransmission();

  Wire.requestFrom(BQ_I2C_ADDR_READ, 2);
  while(!Wire.available());
  byte lsb = Wire.read();
  byte msb = Wire.read();

  debugPrint(F("[+] Sub Cmd uint16_t RESP at 0x40 -> "));
  debugPrintlnCmd((uint16_t)(msb << 8 | lsb));

  return (unsigned int)(msb << 8 | lsb);
}

void bq76952::enterConfigUpdate(void) {
  subCommand(0x0090);
  delayMicroseconds(2000);
}

void bq76952::exitConfigUpdate(void) {
  subCommand(0x0092);
  delayMicroseconds(1000);
}

// CHECKSUM = ~(SUM OF ALL DATA BYTES)
////////////////////////////////////////////////////////////////////////////////////////////////////////


/////// API FUNCTIONS ///////

bq76952::bq76952(byte alertPin) {
	// Constructor
  pinMode(alertPin, INPUT);
  // TODO - Attach IRQ here
}

void bq76952::begin(void) {
  initBQ();
  if(BQ_DEBUG) {
    Serial.begin(DBG_BAUD);
    debugPrintln(F("[+] Initializing BQ76952..."));
  }
}

bool bq76952::isConnected(void) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  if(Wire.endTransmission() == 0) {
    debugPrintln(F("[+] BQ76592 -> Connected on I2C"));
    return true;
  }
  else {
    debugPrintln(F("[+] BQ76592 -> Not Detected on I2C"));
    return false;
  }
}

// Reset the BQ chip
void bq76952::reset(void) {
  subCommand(0x0012);
  debugPrintln(F("[+] Resetting BQ76952..."));
}

// Read single cell voltage
unsigned int bq76952::getCellVoltage(byte cellNumber) {
  return directCommand(CELL_NO_TO_ADDR(cellNumber));
}

// Read All cell voltages in given array - Call like readAllCellVoltages(&myArray)
void bq76952::getAllCellVoltages(unsigned int* cellArray) {
  for(byte x=1;x<17;x++)
    cellArray[x] = getCellVoltage(x);
}

// Measure CC2 current
unsigned int bq76952::getCurrent(void) {
  return directCommand(CMD_DIR_CC2_CUR);
}

// Measure chip temperature in °C
float bq76952::getInternalTemp(void) {
  float raw = directCommand(CMD_DIR_INT_TEMP)/10.0;
  return (raw - 273.15);
}

// Measure thermistor temperature in °C
float bq76952::getThermistorTemp(bq76952_thermistor thermistor) {
  byte cmd;
  switch(thermistor) {
    case TS1:
      cmd = 0x70;
      break;
    case TS2:
      cmd = 0x72;
      break;
    case TS3:
      cmd = 0x74;
      break;
    case HDQ:
      cmd = 0x76;
      break;
    case DCHG:
      cmd = 0x78;
      break;
    case DDSG:
      cmd = 0x7A;
      break;
  }
  float raw = directCommand(cmd)/10.0;
  return (raw - 273.15);
}

// Check Primary Protection status
bq76952_protection_t bq76952::getProtectionStatus(void) {
  bq76952_protection_t prot;
  byte regData = (byte)directCommand(CMD_DIR_FPROTEC);
  prot.bits.SC_DCHG = bitRead(regData, BIT_SA_SC_DCHG);
  prot.bits.OC2_DCHG = bitRead(regData, BIT_SA_OC2_DCHG);
  prot.bits.OC1_DCHG = bitRead(regData, BIT_SA_OC1_DCHG);
  prot.bits.OC_CHG = bitRead(regData, BIT_SA_OC_CHG);
  prot.bits.CELL_OV = bitRead(regData, BIT_SA_CELL_OV);
  prot.bits.CELL_OU = bitRead(regData, BIT_SA_CELL_OU);
  return prot;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////



///// UTILITY FUNCTIONS /////

void bq76952::setDebug(bool d) {
  BQ_DEBUG = d;
}

// Debug printing utilites
void bq76952::debugPrint(const char* msg) {
  if(BQ_DEBUG)
    Serial.print(msg);
}

void bq76952::debugPrintln(const char* msg) {
  if(BQ_DEBUG)
    Serial.println(msg);
}

void bq76952::debugPrint(const __FlashStringHelper* msg) {
  if(BQ_DEBUG)
    Serial.print(msg);
}

void bq76952::debugPrintln(const __FlashStringHelper* msg) {
  if(BQ_DEBUG)
    Serial.println(msg);
}

void bq76952::debugPrintlnCmd(unsigned int cmd) {
  if(BQ_DEBUG) {
    Serial.print(F("0x"));
    Serial.println(cmd, HEX);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////