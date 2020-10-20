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
#define CMD_DIR_SUBCMD_LOW            0x3E
#define CMD_DIR_SUBCMD_HI             0x3F
#define CMD_DIR_RESP_LEN              0x61
#define CMD_DIR_RESP_START            0x40
#define CMD_DIR_RESP_CHKSUM           0x60

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
#define CMD_DIR_STEMP             0x04
#define CMD_DIR_FTEMP             0x05
#define CMD_DIR_SFET              0x06
#define CMD_DIR_FFET              0x07
#define CMD_DIR_VCELL_1           0x14
#define CMD_DIR_INT_TEMP          0x68
#define CMD_DIR_CC2_CUR           0x3A
#define CMD_DIR_FET_STAT          0x7F

// Alert Bits in BQ76952 registers
#define BIT_SA_SC_DCHG            7
#define BIT_SA_OC2_DCHG           6
#define BIT_SA_OC1_DCHG           5
#define BIT_SA_OC_CHG             4
#define BIT_SA_CELL_OV            3
#define BIT_SA_CELL_UV            2

#define BIT_SB_OTF                7
#define BIT_SB_OTINT              6
#define BIT_SB_OTD                5
#define BIT_SB_OTC                4
#define BIT_SB_UTINT              2
#define BIT_SB_UTD                1
#define BIT_SB_UTC                0

// Inline functions
#define CELL_NO_TO_ADDR(cellNo) (0x14 + ((cellNo-1)*2))
#define LOW_BYTE(data) (byte)(data & 0x00FF)
#define HIGH_BYTE(data) (byte)((data >> 8) & 0x00FF)

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
  Wire.write(CMD_DIR_SUBCMD_LOW);
  Wire.write((byte)data & 0x00FF);
  Wire.write((byte)(data >> 8) & 0x00FF);
  Wire.endTransmission();

  debugPrint(F("[+] Sub Cmd SENT to 0x3E -> "));
  debugPrintlnCmd((uint16_t)data);
}

// Read subcommand response
unsigned int bq76952::subCommandResponseInt(void) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(CMD_DIR_RESP_START);
  Wire.endTransmission();

  Wire.requestFrom(BQ_I2C_ADDR_READ, 2);
  while(!Wire.available());
  byte lsb = Wire.read();
  byte msb = Wire.read();

  debugPrint(F("[+] Sub Cmd uint16_t RESP at 0x40 -> "));
  debugPrintlnCmd((uint16_t)(msb << 8 | lsb));

  return (unsigned int)(msb << 8 | lsb);
}

// Enter config update mode
void bq76952::enterConfigUpdate(void) {
  subCommand(0x0090);
  delayMicroseconds(2000);
}

// Exit config update mode
void bq76952::exitConfigUpdate(void) {
  subCommand(0x0092);
  delayMicroseconds(1000);
}

// Write Byte to Data memory of BQ76952
void bq76952::writeDataMemory(unsigned int addr, byte data) {
  byte chksum = 0;
  chksum = computeChecksum(chksum, BQ_I2C_ADDR_WRITE);
  chksum = computeChecksum(chksum, CMD_DIR_SUBCMD_LOW);
  chksum = computeChecksum(chksum, LOW_BYTE(addr));
  chksum = computeChecksum(chksum, HIGH_BYTE(addr));
  chksum = computeChecksum(chksum, data);

  enterConfigUpdate();
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(CMD_DIR_SUBCMD_LOW);
  Wire.write(LOW_BYTE(addr));
  Wire.write(HIGH_BYTE(addr));
  Wire.write(data);
  Wire.endTransmission();

  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(CMD_DIR_RESP_CHKSUM);
  Wire.write(chksum);
  Wire.write(0x05);
  Wire.endTransmission();
  exitConfigUpdate();
}

// Read Byte from Data memory of BQ76952
byte bq76952::readDataMemory(unsigned int addr) {
  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(CMD_DIR_SUBCMD_LOW);
  Wire.write(LOW_BYTE(addr));
  Wire.write(HIGH_BYTE(addr));
  Wire.endTransmission();

  Wire.beginTransmission(BQ_I2C_ADDR_WRITE);
  Wire.write(CMD_DIR_RESP_START);
  Wire.endTransmission();

  Wire.requestFrom(BQ_I2C_ADDR_READ, 1);
  while(!Wire.available());
  return (byte)Wire.read();
}

// Compute checksome = ~(sum of all bytes)
byte bq76952::computeChecksum(byte oldChecksum, byte data) {
  if(!oldChecksum)
    oldChecksum = data;
  else
    oldChecksum = ~(oldChecksum) + data;
  return ~(oldChecksum);
}

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
  prot.bits.CELL_UV = bitRead(regData, BIT_SA_CELL_UV);
  return prot;
}

// Check Temperature Protection status
bq76952_temperature_t bq76952::getTemperatureStatus(void) {
  bq76952_temperature_t prot;
  byte regData = (byte)directCommand(CMD_DIR_FTEMP);
  prot.bits.OVERTEMP_FET = bitRead(regData, BIT_SB_OTC);
  prot.bits.OVERTEMP_INTERNAL = bitRead(regData, BIT_SB_OTINT);
  prot.bits.OVERTEMP_DCHG = bitRead(regData, BIT_SB_OTD);
  prot.bits.OVERTEMP_CHG = bitRead(regData, BIT_SB_OTC);
  prot.bits.UNDERTEMP_INTERNAL = bitRead(regData, BIT_SB_UTINT);
  prot.bits.UNDERTEMP_DCHG = bitRead(regData, BIT_SB_UTD);
  prot.bits.UNDERTEMP_CHG = bitRead(regData, BIT_SB_UTC);
  return prot;
}

void bq76952::setFET(bq76952_fet fet, bq76952_fet_state state) {
  unsigned int subcmd;
  switch(state) {
    case OFF:
      switch(fet) {
        case DCHG:
          subcmd = 0x0093;
          break;
        case CHG:
          subcmd = 0x0094;
          break;
        default:
          subcmd = 0x0095;
          break;
      }
      break;
    case ON:
      subcmd = 0x0096;
      break;
  }
  subCommand(subcmd);
}

// is Charging FET ON?
bool bq76952::isCharging(void) {
  byte regData = (byte)directCommand(CMD_DIR_FET_STAT);
  if(regData & 0x01) {
    debugPrintln(F("[+] Charging FET -> ON"));
    return true;
  }
  debugPrintln(F("[+] Charging FET -> OFF"));
  return false;
}

// is Discharging FET ON?
bool bq76952::isDischarging(void) {
  byte regData = (byte)directCommand(CMD_DIR_FET_STAT);
  if(regData & 0x04) {
    debugPrintln(F("[+] Discharging FET -> ON"));
    return true;
  }
  debugPrintln(F("[+] Discharging FET -> OFF"));
  return false;
}

// Set user-defined overvoltage protection
void bq76952::setCellOvervoltageProtection(unsigned int mv, unsigned int ms) {
  byte thresh = (byte)mv/50.6;
  uint16_t dly = (uint16_t)(ms/3.3)-2;
  if(thresh < 20 || thresh > 110)
    thresh = 86;
  else {
    debugPrint(F("[+] COV Threshold => "));
    debugPrintlnCmd(thresh);
    writeDataMemory(0x9278, thresh);
  }
  if(dly < 1 || dly > 2047)
    dly = 74;
  else {
    debugPrint(F("[+] COV Delay => "));
    debugPrintlnCmd(dly);
    writeDataMemory(0x9279, dly);
  }
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