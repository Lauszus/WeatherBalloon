/* Copyright (C) 2013 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#include "GSMSIM300.h"

const char* receiveSmsString = "+CMTI: \"SM\","; // +CMTI: "SM",index\r\n
const char* incomingCallString = "RING";
const char* hangupCallString = "NO CARRIER";
const char* powerDownString = "NORMAL POWER DOWN";

// TODO: Remove all delays

GSMSIM300::GSMSIM300(const char *pinCode, uint8_t rx, uint8_t tx, uint8_t powerPin) :
pinCode(pinCode),
powerPin(powerPin),
pGsmString(gsmString),
pOutString(outString),
pReceiveSmsString((char*)receiveSmsString),
pIncomingCallString((char*)incomingCallString),
pHangupCallString((char*)hangupCallString),
pPowerDownString((char*)powerDownString),
readIndex(false),
_newSMS(false)
{
  pinMode(powerPin,OUTPUT);
  digitalWrite(powerPin,HIGH);

  gsm = new SoftwareSerial(rx, tx);
  gsm->begin(9600);
#ifdef DEBUG
  Serial.println(F("GSM started"));
#endif
  gsmState = GSM_POWER_ON;
  smsState = SMS_IDLE;
  callState = CALL_IDLE;
}

void GSMSIM300::update() {
    incomingChar = gsm->read();
#ifdef EXTRADEBUG
    if(incomingChar != -1)
      Serial.write(incomingChar);
#endif
    if (checkString(powerDownString,&pPowerDownString)) {
#ifdef DEBUG
      Serial.println(F("GSM Module turned off"));
#endif
      gsmState = GSM_POWER_ON;
    }

    switch(gsmState) {
      case GSM_POWER_ON:
        delay(1000);
        gsm->print(F("AT+CPOWD=0\r")); // Turn off the module if it's already on
#ifdef DEBUG
        Serial.println(F("GSM PowerOn"));
#endif
        delay(2000);
        powerOn();
#ifdef DEBUG
        Serial.println(F("GSM check state"));
#endif
        gsm->print(F("AT\r"));
        delay(500);
        gsm->print(F("AT\r"));
        setGsmWaitingString("OK");
        gsmState = GSM_POWER_ON_WAIT;
        break;
        
      case GSM_POWER_ON_WAIT:
        if(checkGsmWaitingString()) {
#ifdef DEBUG
          Serial.println(F("GSM Module is powered on\r\nChecking SIM Card"));
#endif
          gsm->print(F("AT+CPIN="));
          gsm->print(pinCode); // Set pin
          gsm->print(F("\r"));
          setGsmWaitingString("Call Ready");
          gsmState = GSM_SET_PIN;
        }
        break;
        
      case GSM_SET_PIN:
        if(checkGsmWaitingString()) {
#ifdef DEBUG
          Serial.println(F("SIM Card ready"));
          Serial.println(F("Waiting for GSM to get ready"));
#endif
          gsmState = GSM_CHECK_CONNECTION;
        }
        break;
        
      case GSM_CHECK_CONNECTION:
#ifdef EXTRADEBUG
        Serial.println(F("Checking Connection"));
#endif
        gsm->print(F("AT+CREG?\r"));
        setGsmWaitingString("+CREG: 0,");
        gsmState = GSM_CHECK_CONNECTION_WAIT;
        break;
        
      case GSM_CHECK_CONNECTION_WAIT:
        if(checkGsmWaitingString())
          gsmState = GSM_CONNECTION_RESPONSE;
        break;
        
      case GSM_CONNECTION_RESPONSE:
        if(incomingChar != -1) {
#ifdef EXTRADEBUG
          Serial.print(F("Connection response: "));
          Serial.println(incomingChar);
#endif
          if(incomingChar != '1') {
            delay(1000);
            gsmState = GSM_CHECK_CONNECTION;
          } else {
#ifdef DEBUG
            Serial.println(F("GSM module is up and running!\r\n"));
#endif
            gsmState = GSM_RUNNING;
          }
        }
        break;
        
      case GSM_RUNNING:
        updateSMS();
        updateCall();
        if(incomingChar != -1) {
          if (checkSMS()) // Return true if a new SMS is received
            _newSMS = true;
          if (checkString(incomingCallString,&pIncomingCallString)) {
#ifdef DEBUG
            Serial.println(F("Incoming Call"));
#endif
            answer();
          }
        }
        break;
      
      case GSM_POWER_OFF:
#ifdef DEBUG
        Serial.println(F("Shutting down GSM module"));
#endif
        gsm->print(F("AT+CPOWD=1\r")); // Turn off the module if it's already on
        setGsmWaitingString(powerDownString);
        gsmState = GSM_POWER_OFF_WAIT;
        break;
      
      case GSM_POWER_OFF_WAIT:
        if(checkGsmWaitingString()) {
          powerOff();
#ifdef DEBUG
          Serial.println(F("GSM PowerOff"));
#endif
          gsmState = GSM_POWER_ON;
        }
        break;
        
      default:
        break;
    }
}

bool GSMSIM300::checkSMS() {
  if (readIndex) {
    if (incomingChar == '\r') { // End of index
      readIndex = false;
#ifdef DEBUG
      Serial.print(F("Received SMS at index: "));
      Serial.println(lastIndex);
#endif
      return true;
    } else if (indexCounter < sizeof(lastIndex)/sizeof(lastIndex[0])-1) {
      //Serial.println(F("Storing index"));
      lastIndex[indexCounter] = incomingChar;
      lastIndex[indexCounter+1] = '\0';
      indexCounter++;
    } else {
#ifdef DEBUG
      Serial.println(F("Index is too long"));
#endif
      readIndex = false;
    }
  } 
  else if (checkString(receiveSmsString,&pReceiveSmsString)) {
    readIndex = true;
    indexCounter = 0;
    //Serial.println(F("Received SMS"));
  }
  return false;
}

// I know this might seem confusing, but in order to change the pointer I have to create a pointer to a pointer
// **pString will get the value of the original pointer, while
// *pString will get the address of the original pointer
bool GSMSIM300::checkString(const char *cmpString, char **pString) {
  if (incomingChar == **pString) {
    (*pString)++;
    if (**pString == '\0')
      return true;
  } else
    *pString = (char*)cmpString; // Reset pointer to start of string
  return false;
}

// TODO: Check if updateSMS or updateCall is caught in a loop
void GSMSIM300::updateSMS() {
  switch(smsState) {
    case SMS_IDLE:
      break;
      
    case SMS_MODE:
#ifdef DEBUG
      Serial.println(F("SMS setting text mode"));
#endif
      gsm->print(F("AT+CMGF=1\r")); // Set SMS type to text mode
      setOutWaitingString("OK");
      smsState = SMS_ALPHABET;
      break;
      
    case SMS_ALPHABET:
      if(checkOutWaitingString()) {
#ifdef DEBUG
        Serial.println(F("SMS setting alphabet"));
#endif
        gsm->print(F("AT+CSCS=\"GSM\"\r")); // GSM default alphabet
        setOutWaitingString("OK");
        smsState = SMS_NUMBER;
      }
      break;
      
    case SMS_NUMBER:
      if(checkOutWaitingString()) {
#ifdef DEBUG
        Serial.print(F("Number: "));
        Serial.println(numberBuffer);
#endif
        gsm->print(F("AT+CMGS=\""));
        gsm->print(numberBuffer);
        gsm->print(F("\"\r"));
        setOutWaitingString(">");
        smsState = SMS_CONTENT;
      }
      break;
      
    case SMS_CONTENT:
      if(checkOutWaitingString()) {
#ifdef DEBUG
        Serial.print(F("Message: \""));
        Serial.print(messageBuffer);
        Serial.println("\"");
#endif
        gsm->print(messageBuffer);
        gsm->write(26); // CTRL-Z
        setOutWaitingString("OK");
        smsState = SMS_WAIT;
      }
      break;
      
    case SMS_WAIT:
      if(checkOutWaitingString()) {
#ifdef DEBUG
        Serial.println(F("SMS is sent"));
#endif
        smsState = SMS_IDLE;
      }
      break;
      
    default:
      break;
  }
}

void GSMSIM300::updateCall() {
  switch(callState) {
    case CALL_IDLE:
      break;
      
    case CALL_NUMBER:
#ifdef DEBUG
      Serial.print(F("Calling: "));
      Serial.println(numberBuffer);
#endif
      gsm->print(F("ATD")); // Dial
      gsm->print(numberBuffer);
      gsm->print(F(";\r"));
      callState = CALL_SETUP;
#ifdef DEBUG
      Serial.print(F("Waiting for connection"));
#endif
      break;
      
    case CALL_SETUP:
#if defined(DEBUG)
      Serial.print(F("."));
#elif EXTRADEBUG
      Serial.print(F("\r\nChecking response"));
#endif
      gsm->print(F("AT+CLCC\r"));
      setOutWaitingString("+CLCC: 1,0,");
      callState = CALL_SETUP_WAIT;
      break;
      
    case CALL_SETUP_WAIT:
      if(checkOutWaitingString()) {
#ifdef EXTRADEBUG
        Serial.print(F("\r\nGot first response"));
#endif
        callState = CALL_RESPONSE;
      }
      break;
      
    case CALL_RESPONSE:
      if(incomingChar != -1) {
#ifdef EXTRADEBUG
        Serial.print(F("\r\nConnection response: "));
        Serial.print(incomingChar);
#endif
        if(incomingChar != '0') {
          delay(1000);
          callState = CALL_SETUP;
        } else {
#ifdef DEBUG
          Serial.println(F("\r\nCall active"));
#endif
          callState = CALL_ACTIVE;
        }
      }
      break;
      
    case CALL_ACTIVE:
      if(incomingChar != -1) {
        if (checkString(hangupCallString,&pHangupCallString)) {
#ifdef DEBUG
          Serial.println(F("Call hangup!"));
#endif
          callState = CALL_IDLE;
        }
      }
      break;
      
    default:
      break;
  }
}

void GSMSIM300::setGsmWaitingString(const char* str) {
  strcpy(gsmString,str);
  pGsmString = gsmString;
  gsmTimer = millis();
}

void GSMSIM300::setOutWaitingString(const char* str) {
  strcpy(outString,str);
  pOutString = outString;
}

// TODO: Combine these two function
bool GSMSIM300::checkGsmWaitingString() {
  if(incomingChar != -1) {
    if(incomingChar == *pGsmString) {
      pGsmString++;
      if(*pGsmString == '\0') {
#ifdef EXTRADEBUG
        Serial.print(F("\r\nGSM Response success: "));
        Serial.write((uint8_t*)gsmString, strlen(gsmString));
        Serial.println();
#endif
        return true;
      }
    }
  }
  if(millis() - gsmTimer > 10000) { // Only wait 10s for response    
    if(gsmState != GSM_RUNNING) {
#ifdef DEBUG
      Serial.println("\r\nNo response from GSM module\r\nResetting...");
#endif
      gsmState = GSM_POWER_ON;
    }
  }
  return false;
}

bool GSMSIM300::checkOutWaitingString() {
  if(incomingChar != -1) {
    if(incomingChar == *pOutString) {
      pOutString++;
      if(*pOutString == '\0') {
#ifdef EXTRADEBUG
        Serial.print(F("\r\nOut Response success: "));
        Serial.write((uint8_t*)outString, strlen(outString));
        Serial.println();
#endif
        return true;
      }
    }
  }
  return false;
}

void GSMSIM300::call(const char* num) {
  strcpy(numberBuffer,num);
  callState = CALL_NUMBER;
}
/*
void GSMSIM300::gsmActivateAutoAnswer() {
  gsm->print(F("ATS0=001\r")); // 'RING' will be received on an incoming call  
}
*/
void GSMSIM300::hangup() {
  gsm->print(F("ATH\r")); // Response: 'OK'
#ifdef DEBUG
  Serial.println(F("Call hangup!"));
#endif
  callState = CALL_IDLE;
}

void GSMSIM300::answer() {
  gsm->print(F("ATA\r"));
  callState = CALL_ACTIVE;
}

//gsm->print(F("AT+CSQ\r")); // Check signal strength - response: 'OK' and then the information

void GSMSIM300::sendSMS(const char* num, const char* mes) {
  strcpy(numberBuffer,num);
  strcpy(messageBuffer,mes);
  smsState = SMS_MODE;
}

bool GSMSIM300::readSMS(char* index) {
  if (index == NULL) {
    if (lastIndex == NULL)
      return false;    
    index = lastIndex;
  }
  _newSMS = false;
  gsm->print(F("AT+CMGF=1\r"));
  gsm->print(F("AT+CMGR="));
  gsm->print(index);
  gsm->print(F("\r"));
  
  uint32_t startTime = millis();
  int8_t i = -1;
  boolean numberFound = false;
  boolean reading = false;
  
  // Read the senders number. The SMS is returned as:
  // +CMGR: "REC UNREAD","number",,"date"
  // content
  
  while(millis() - startTime < 1000) { // Only do this for 1s
    while(!gsm->available());
    char c = gsm->read();
#ifdef EXTRADEBUG
    Serial.write(c);
#endif
    if (reading) {
      if (i > -1) {
        if (c == '"') { // Second "
#if defined(DEBUG) && !defined(EXTRADEBUG)
          Serial.print(F("Extracted the following number: "));
          Serial.println(numberBuffer);
#endif
          numberFound = true;
          break;
        }
        // TODO: Gem i dedikeret buffer
        numberBuffer[i] = c;
        numberBuffer[i+1] = '\0';
      }
      i++; // TODO: Check længden af i
    } else if (c == ',')
      reading = true;
  }
  
  startTime = millis();
  reading = false;
  
  while(millis() - startTime < 1000) { // Only do this for 1s
    while(!gsm->available());
    char c = gsm->read();
#ifdef EXTRADEBUG
    Serial.write(c);
#endif
    if (reading) {
      // TODO: Gem i messageBuffer
#if defined(DEBUG) && !defined(EXTRADEBUG)
      Serial.write(c);
#endif
      if (c == '\n') // End of second string
        break;
    }
    else if (c == '\n') { // End of first string
      reading = true;
#ifdef DEBUG
      Serial.print(F("Received message: "));
#endif
    }
  }
  // TODO: 'return numberFound && messageFound;'
  return numberFound;
}

void GSMSIM300::powerOn() {
  digitalWrite(powerPin,LOW);
  delay(4000);
  digitalWrite(powerPin,HIGH);
  delay(2000);
}
void GSMSIM300::powerOff() {  
  digitalWrite(powerPin,LOW);
  delay(800);
  digitalWrite(powerPin,HIGH);
  delay(6000);
}