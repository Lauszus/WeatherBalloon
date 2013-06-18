#ifdef ENABLE_PRESSURE

#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

void pressureInit() {
  if (bmp.begin())
    Serial.println(F("BMP085 started"));
  else
    Serial.println(F("Could not find a valid BMP085 sensor!"));
}

void printPressure() {
  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature());
  Serial.println(F(" *C"));

  Serial.print(F("Pressure = "));
  Serial.print(bmp.readPressure());
  Serial.println(F(" Pa"));

  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  Serial.print(F("Altitude = "));
  Serial.print(bmp.readAltitude());
  Serial.println(F(" meters"));

  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
  /*Serial.print(F("Real altitude = "));
  Serial.print(bmp.readAltitude(101500));
  Serial.println(F(" meters"));*/

  Serial.println();
}

#endif

