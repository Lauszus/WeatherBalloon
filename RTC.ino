#ifdef ENABLE_SD_RTC

#include <RTClib.h>

RTC_DS1307 RTC;

void timeInit() {
  RTC.begin();

  if (!RTC.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    // Following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }  
}

void getTimestamp(char* buffer) {
  DateTime now = RTC.now();
  
  sprintf(buffer, "%d/%d/%d %d:%d:%d", now.day(),now.month(),now.year(),now.hour(),now.minute(),now.second());
  
/*
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);

  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
*/
  /*Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());*/
}

#endif
