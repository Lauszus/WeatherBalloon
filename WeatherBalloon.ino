#define ENABLE_GSM
//#define ENABLE_GPS
//#define ENABLE_PRESSURE
//#define ENABLE_TEMP
//#define ENABLE_SD_RTC

#include <Wire.h>
#include <SoftwareSerial.h>
#include <MemoryFree.h>
#include "GSMSIM300.h" // My GSM library
#include "WeatherBalloon.h"
#include "secret.h" // Contains our numbers, pin code etc.

uint32_t timer;

GSMSIM300 GSM(pinCode,2,3,4); // Pin code, rx, tx, powerPin

void setup() {
  Serial.begin(115200);
  Wire.begin(); // All calls to Wire.begin has been removed in the libraries, so it's only called once
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
  GSM.update();
  if (GSM.gsmState == GSM_RUNNING) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'C')
        GSM.call(number);
      else if (c == 'H')
        GSM.callHangup();
      else if (c == 'S')
        GSM.sendSMS(number, "Send lige en sms :)");
      else if (c == 'R')
        GSM.readSMS();
    }
    if(GSM.newSMS()) {
      if(GSM.readSMS()) // Returns true if the number of the sender is successfully extracted from the SMS
        GSM.sendSMS(GSM.numberBuffer, "Automatic response from WeatherBallon\nMy coordinates are: (lat,lng)");
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

