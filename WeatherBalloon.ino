#define ENABLE_GSM
//#define ENABLE_GPS
//#define ENABLE_PRESSURE
//#define ENABLE_TEMP
//#define ENABLE_SD_RTC

//#define DEBUG

#include <Wire.h>
#include <SoftwareSerial.h>
#include <MemoryFree.h>
#include "WeatherBalloon.h"
#include "secret.h" // Contains our numbers, pin code etc.

uint32_t timer;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // All calls to Wire.begin has been removed in the libraries, so it's only called once
  gsmInit();
  //gpsInit();  
  //pressureInit();
  //timeInit();
  //sdInit();
  
  Serial.print(F("Free memory: "));
  Serial.println(freeMemory());
  Serial.println(F("WeatherBalloon is running!\r\n"));
  timer = millis();
}

void loop() {
  updateGsm();
  if (gsmState == GSM_RUNNING) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'C')
        call(number);
      else if (c == 'H' && callState == CALL_ACTIVE)
        callHangup();
      else if (c == 'S')
        sendSMS(number, "Nu skulle det gerne virke :)");
      else if (c == 'R')
        readSMS(lastIndex);
    }
  }
  /*
  if (Serial.available()) {
      char c = Serial.read();
      Serial.println(c);
      if (c == 'W')
        writePosToSd();
  }
  */
  /*
  uint32_t ms = millis();
  if (ms - timer > 2000) {
    timer = ms;    
    //printTemp();
    //printPressure();
    printGps();
  }
  */
}

