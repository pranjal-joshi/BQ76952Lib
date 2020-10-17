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

class bq76952 {
public:
	bq76952();
	void begin(void);
	void begin(bool);
	unsigned int readCellVoltage(byte cellNumber);
	void readAllCellVoltages(unsigned int* cellArray);
	unsigned int readCurrent(void);
	void debugPrint(const char*);
	void debugPrintln(const char*);
	void debugPrint(const __FlashStringHelper*);
	void debugPrintln(const __FlashStringHelper*);
private:
	void initBQ(void);
	void directCommand(byte);
	void subCommand(byte, unsigned int);
};