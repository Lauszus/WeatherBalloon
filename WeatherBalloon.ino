#define ENABLE_GSM
//#define ENABLE_GPS
//#define ENABLE_PRESSURE
//#define ENABLE_TEMP
//#define ENABLE_SD_RTC

//#include <Wire.h>
#include <SoftwareSerial.h> // Please increase rx buffer to 256
#include <MemoryFree.h>
#include "GSMSIM300.h" // My GSM library
#include "WeatherBalloon.h"
#include "secret.h" // Contains our numbers, pin code etc.

uint32_t timer;

GSMSIM300 GSM(pinCode,2,3,4,false); // Pin code, rx, tx, powerPin, set to true if the module is already running

void setup() {
  Serial.begin(115200);
  GSM.begin(28800); // I have found this baud rate to work pretty well
  //Wire.begin(); // All calls to Wire.begin has been removed in the libraries, so it's only called once
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
  if (GSM.getState() == GSM_RUNNING) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'C')
        GSM.call(number);
      else if (c == 'H')
        GSM.hangup();
      else if (c == 'S')
        GSM.sendSMS(number, "Send lige en sms :)");
      else if (c == 'R')
        GSM.readSMS();
      else if (c == 'L')
        //GSM.listSMS("REC UNREAD"); // List unread messages, all is the default
        //GSM.listSMS();
        GSM.listSMS("ALL",true);
      else if (c == 'K')
        GSM.listSMS("ALL",false);
    }
    if(GSM.newSMS()) {
      if(GSM.readSMS()) // Returns true if the number of the sender is successfully extracted from the SMS
        GSM.sendSMS(GSM.numberIn, "Automatic response from WeatherBallon\nMy coordinates are: (lat,lng)");
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

