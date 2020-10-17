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
#define ADR_SUBCMD_LOW            0x3E
#define ADR_SUBCMD_HI             0x3F
#define ADR_RESP_LEN              0x61
#define ADR_RESP_START            0x40
#define ADR_RESP_CHKSUM           0x60

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
#define CMD_DIR_VCELL_1           0x14
#define CMD_DIR_INT_TEMP          0x68
#define CMD_DIR_CC2_CUR           0x3A

// Inline functions
#define CELL_NO_TO_ADDR(cellNo) (0x14 + ((cellNo-1)*2))

bq76952::bq76952(unsigned char alertPin) {
	// Constructor
  pinMode(alertPin, INPUT);
  // TODO - Attach IRQ here
}

void bq76952::initBQ(void) {
  Wire.begin();
}

void bq76952::begin(void) {
  initBQ();
  BQ_DEBUG = false;
}

void bq76952::begin(bool dbg) {
  initBQ();
  if(dbg) {
  	BQ_DEBUG = dbg;
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

// Send Direct command
unsigned int bq76952::directCommand(byte command) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(command);
  Wire.endTransmission();

  Wire.requestFrom(BQ_I2C_ADDR_READ, 2);
  while(!Wire.available());
  byte lsb = Wire.read();
  byte msb = Wire.read();
  return (unsigned int)(msb << 8 | lsb);
}

// Send Sub-command
void bq76952::subCommand(byte addr, unsigned int data) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(ADR_SUBCMD_LOW);
  Wire.write((byte)data & 0x00FF);
  Wire.write((byte)(data >> 8) & 0x00FF);
  Wire.endTransmission();
}

// TODO - Implement BLOCK READ after SUBCMD

// Read single cell voltage
unsigned int bq76952::readCellVoltage(byte cellNumber) {
  return directCommand(CELL_NO_TO_ADDR(cellNumber));
}

// Read All cell voltages in given array - Call like readAllCellVoltages(&myArray)
void bq76952::readAllCellVoltages(unsigned int* cellArray) {
  for(byte x=1;x<17;x++)
    cellArray[x] = readCellVoltage(x);
}

unsigned int bq76952::readCurrent(void) {
  return directCommand(CMD_DIR_CC2_CUR);
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