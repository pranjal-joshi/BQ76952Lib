/*
* Description :   Example for using BQ76952 BMS IC (by Texas Instruments) for Arduino platform.
* Author      :   Pranjal Joshi
* Date        :   21/10/2020 
* License     :   MIT
* This code is published as open source software. Feel free to share/modify.
*/

#include <bq76952lib.h>

#define ALERT_PIN 10

bq76952 bms(ALERT_PIN);

void setup() {
  bms.setDebug(true);
  bms.begin();
  bms.reset();
  bq76952_protection_t status = bms.getProtectionStatus();
  bq76952_temperature_t tStat = bms.getTemperatureStatus();
  Serial.println(status.bits.SC_DCHG);
  Serial.println(tStat.bits.OVERTEMP_FET);
  bms.setCellOvervoltageProtection(4150, 1000);
  bms.setCellUndervoltageProtection(2880, 1000);
  bms.setChargingOvercurrentProtection(50, 10);
  bms.setDischargingOvercurrentProtection(70, 15);
  bms.setDischargingShortcircuitProtection(SCD_40, 100);
  bms.setFET(DCH, ON);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10);
}
