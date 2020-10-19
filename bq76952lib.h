/*
* Description :   Header file of BQ76952 BMS IC (by Texas Instruments) for Arduino platform.
* Author      :   Pranjal Joshi
* Date        :   17/10/2020 
* License     :   MIT
* This code is published as open source software. Feel free to share/modify.
*/

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

enum bq76952_thermistor {
	TS1,
	TS2,
	TS3,
	HDQ,
	DCHG,
	DDSG
};

typedef union protection {
	struct {
		SC_DCHG            :1;
		OC2_DCHG           :1;
		OC1_DCHG           :1;
		OC_CHG             :1;
		CELL_OV            :1;
		CELL_UV            :1;
	} bits;
	uint8_t data;
} bq76952_protection_t;

class bq76952 {
public:
	bq76952(byte);
	void begin(void);
	void reset(void);
	bool isConnected(void);
	unsigned int getCellVoltage(byte cellNumber);
	void getAllCellVoltages(unsigned int* cellArray);
	unsigned int getCurrent(void);
	float getInternalTemp(void);
	float getThermistorTemp(bq76952_thermistor);
	bq76952_protection_t getProtectionStatus(void);
	void setDebug(bool);
	void debugPrint(const char*);
	void debugPrintln(const char*);
	void debugPrint(const __FlashStringHelper*);
	void debugPrintln(const __FlashStringHelper*);
	void debugPrintlnCmd(unsigned int cmd);
private:
	void initBQ(void);
	unsigned int directCommand(byte);
	void subCommand(unsigned int);
	unsigned int subCommandResponseInt(void);
	void enterConfigUpdate(void);
	void exitConfigUpdate(void);
};