#ifdef ENABLE_SD_RTC

#include <SD.h>
#include <SPI.h>

File file;

const int chipSelect = 10;

char timestamp[50];
char fileName[30];
uint32_t counter = 0;

char* dir = "logs";

void sdInit() {
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("SD initialization failed!"));
    return;
  }
  
  Serial.print(F("Writing to: "));
  Serial.println(dir);
  SD.mkdir(dir);
  
  Serial.print(F("Current date and time: "));
  getTimestamp(timestamp);
  Serial.println(timestamp);
  
  Serial.println(F("SD initialization done"));
}

void writePosToSd() {
  sprintf(fileName, "%s/log%d.txt", dir, counter++);
  
  if(SD.exists(fileName)) {
    Serial.print(fileName);
    Serial.println(F(" already exist. Deleting file..."));
    SD.remove(fileName);
  }
  
  file = SD.open(fileName, FILE_WRITE);  
  if (file) {
    getTimestamp(timestamp);
    file.println(timestamp);
    file.close(); // Close the file
    Serial.print(F("Wrote: "));
    Serial.print(timestamp);
    Serial.print(F(" to: "));
    Serial.println(fileName);
  } else {
    Serial.print(F("Error opening "));
    Serial.println(fileName);
  }
  Serial.println();
  //delay(1000);
}

#endif
