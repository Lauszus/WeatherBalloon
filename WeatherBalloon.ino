#define ENABLE_GSM
#define ENABLE_GPS
//#define ENABLE_PRESSURE
//#define ENABLE_TEMP
//#define ENABLE_SD_RTC

#include <GSMSIM300.h> // My GSM library
#include <SoftwareSerial.h> // Please increase rx buffer to 256
#include <Wire.h>
#include <MemoryFree.h>
#include <TinyGPS.h>
#include "WeatherBalloon.h"
#include "secret.h" // Contains our numbers and pin code

uint32_t timer;

// The constructor below will start a SoftwareSerial connection on the chosen rx and tx pins
// The power pin is connected to the status pin on the module
// Letting the microcontroller turn the module on and off
GSMSIM300 GSM(pinCode,2,3,4,false); // Pin code, rx, tx, power pin, set to true if the module is already running

void setup() {
  Serial.begin(115200);
  GSM.begin(9600); // Start the GSM library
  //Wire.begin(); // All calls to Wire.begin have been removed in the libraries, so it's only called once
  gpsInit();  
  //pressureInit();
  //timeInit();
  //sdInit();
  
  Serial.print(F("Free memory: "));
  Serial.println(freeMemory());
  Serial.println(F("WeatherBalloon is running!\r\n"));
  timer = millis();
}

void loop() {
  GSM.update(); // This will update the state machine in the library
  if (GSM.getState() == GSM_RUNNING) { // Make sure the GSM module is up and running
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'C')
        GSM.call(number); // Call number
      else if (c == 'H')
        GSM.hangup(); // Hangup conversation
      else if (c == 'S')
        GSM.sendSMS(number, "Send lige en sms :)"); // Send SMS
      else if (c == 'R')
        GSM.readSMS(); // Read the last returned SMS
      else if (c == 'L')
        GSM.listSMS("ALL"); // List all messages
    }
    if(GSM.newSMS()) { // Check if a new SMS is received
      if(GSM.readSMS()) { // Returns true if the number and message of the sender is successfully extracted from the SMS
        //GSM.sendSMS(GSM.numberIn, "Automatic response from WeatherBallon\nMy coordinates are: (lat,lng)"); // Sends a response to that number
        if (strcmp(GSM.messageIn,"CMD") == 0) {
          if (flat != TinyGPS::GPS_INVALID_F_ANGLE && flon != TinyGPS::GPS_INVALID_F_ANGLE) {
            char buf[50];
            sprintf(buf, "My coordinates are: %f,%f",flat,flon);
            GSM.sendSMS(GSM.numberIn, buf);
          } else
            GSM.sendSMS(GSM.numberIn, "Still waiting for fix");
        }
      }
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
  uint32_t ms = millis();
  if (ms - timer > 2000) {
    timer = ms;    
    //printTemp();
    //printPressure();
    printGps();
  }
}
