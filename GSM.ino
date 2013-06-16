SoftwareSerial gsm(2, 3); // RX, TX

void gsmInit() {
  pinMode(powerPin,OUTPUT);
  digitalWrite(powerPin,HIGH);
  pinMode(LED,OUTPUT);
  
  gsm.begin(9600);
  Serial.println(F("GSM started"));
  gsmState = GSM_POWER_ON;
  smsState = SMS_IDLE;
  callState = CALL_IDLE;
}

void updateGsm() {
    switch(gsmState) {
      case GSM_POWER_ON:
        delay(1000);
        gsm.print(F("AT+CPOWD=0\r")); // Turn off the module if it's already on
        Serial.println(F("GSM PowerOn"));
        delay(2000);
        gsmPowerOn();
        
        Serial.println(F("GSM check state"));
        gsm.print(F("AT\r"));
        delay(500);
        gsm.print(F("AT\r"));
        setGsmWaitingString("OK");
        gsmState = GSM_POWER_ON_WAIT;
        break;
        
      case GSM_POWER_ON_WAIT:
        if(checkGsmWaitingString()) {
          Serial.println(F("GSM Module is powered on\r\nChecking SIM Card"));
          gsm.print(F("AT+CPIN="));
          gsm.print(pinCode); // Set pin
          gsm.print(F("\r"));
          setGsmWaitingString("Call Ready");
          gsmState = GSM_SET_PIN;
        }
        break;
        
      case GSM_SET_PIN:
        if(checkGsmWaitingString()) {
          Serial.println(F("SIM Card ready"));
          gsmState = GSM_CHECK_CONNECTION;
        }
        break;
        
      case GSM_CHECK_CONNECTION:
        Serial.println(F("Checking Connection..."));
        gsm.print(F("AT+CREG?\r"));
        setGsmWaitingString("+CREG: 0,");
        gsmState = GSM_CHECK_CONNECTION_WAIT;
        break;
        
      case GSM_CHECK_CONNECTION_WAIT:
        if(checkGsmWaitingString())
          gsmState = GSM_CONNECTION_RESPONSE;
        break;
        
      case GSM_CONNECTION_RESPONSE:
        if(gsm.available()) {
          char c = gsm.read();
          Serial.print(F("Connection response: "));
          Serial.println(c);
          if(c != '1') {
            delay(1000);
            gsmState = GSM_CHECK_CONNECTION;
          } else {
            Serial.println(F("GSM module is up and running!"));
            gsmState = GSM_RUNNING;
          }
        }
        break;
        
      case GSM_RUNNING:
        updateSMS();
        updateCall();
        if(smsState == SMS_IDLE && callState == CALL_IDLE) {          
          if(gsm.available()) {
            char c = gsm.read();
#ifdef DEBUG
            Serial.write(c);
            /*if (c != '\n')
              Serial.println();
            Serial.println((uint8_t)c);*/
#endif
            checkSMS(c);            
          }
        }
        break;
      
      case GSM_POWER_OFF:
        Serial.println(F("Shutting down GSM module"));
        gsm.print(F("AT+CPOWD=1\r")); // Turn off the module if it's already on
        setGsmWaitingString("NORMAL POWER DOWN");
        gsmState = GSM_POWER_OFF_WAIT;
        break;        
      
      case GSM_POWER_OFF_WAIT:
        if(checkGsmWaitingString()) {
          gsmPowerOff();
          Serial.println(F("GSM PowerOff"));
          gsmState = GSM_POWER_ON;
        }
        break;
        
      default:
        break;
    }
}

void checkSMS(char c) {
  if (readIndex) {
    if (c == '\r') { // End of index
      readIndex = false;
      Serial.print(F("Received SMS at index: "));
      Serial.println(lastIndex);
    } else if (indexCounter < sizeof(lastIndex)/sizeof(lastIndex[0])-1) {
      Serial.println(F("Storing index"));
      lastIndex[indexCounter] = c;
      lastIndex[indexCounter+1] = '\0';
      indexCounter++;
    } else {
      Serial.println(F("Index is too long"));
      readIndex = false;
    }
  } else if (c == *pReceiveSmsString) {
    pReceiveSmsString++;
    if (*pReceiveSmsString == '\0') {
      readIndex = true;
      indexCounter = 0;
      Serial.println(F("Received SMS"));
    }
  } else
    pReceiveSmsString = receiveSmsString;
}

void updateSMS() {
  switch(smsState) {
    case SMS_IDLE:
      break;
      
    case SMS_MODE:
      Serial.println(F("SMS setting text mode"));
      
      gsm.print(F("AT+CMGF=1\r")); // Set sms type to text mode
      setOutWaitingString("OK");
      smsState = SMS_ALPHABET;
      break;
      
    case SMS_ALPHABET:
      if(checkOutWaitingString()) {
        Serial.println(F("SMS setting alphabet"));
        
        gsm.print(F("AT+CSCS=\"GSM\"\r")); // GSM default alphabet
        setOutWaitingString("OK");
        smsState = SMS_NUMBER;
      }
      break;
      
    case SMS_NUMBER:
      if(checkOutWaitingString()) {
        Serial.print(F("Number: "));
        Serial.println(number);
        
        gsm.print(F("AT+CMGS=\""));
        gsm.print(number);
        gsm.print(F("\"\r"));
        setOutWaitingString(">");
        smsState = SMS_CONTENT;
      }
      break;
      
    case SMS_CONTENT:
      if(checkOutWaitingString()) {
        Serial.print(F("Message: "));
        Serial.println(message);
        
        gsm.print(message);
        gsm.write(26); // CTRL-Z
        setOutWaitingString("OK");
        smsState = SMS_WAIT;
      }
      break;
      
    case SMS_WAIT:
      if(checkOutWaitingString()) {
        Serial.println(F("SMS is Send"));
        smsState = SMS_IDLE;
      }
      break;
      
    default:
      break;
  }
}

void updateCall() {
  switch(callState) {
    case CALL_IDLE:
      break;
      
    case CALL_NUMBER:
      Serial.print(F("Calling: "));
      Serial.println(number);

      gsm.print(F("ATD"));
      gsm.print(number);
      gsm.print(F(";\r"));
      callState = CALL_SETUP;
      break;
      
    case CALL_SETUP:
      Serial.println(F("\r\nWaiting for connection..."));

      gsm.print(F("AT+CLCC\r"));
      setOutWaitingString("+CLCC: 1,0,");
      callState = CALL_SETUP_WAIT;
      break;
      
    case CALL_SETUP_WAIT:
      if(checkOutWaitingString()) {
        Serial.println(F("Checking response"));
        callState = CALL_RESPONSE;
      }
      break;
      
    case CALL_RESPONSE:
      if(gsm.available()) {
        char c = gsm.read();
        Serial.print(F("Connection response: "));
        Serial.println(c);
        if(c != '0') {
          delay(1000);
          callState = CALL_SETUP;
        } else {
          Serial.print(F("Call active"));
          callState = CALL_ACTIVE;
        }
      }
      break;
      
    case CALL_ACTIVE:
      if(gsm.available())
        Serial.write(gsm.read());
      break;
      
    default:
      break;
  }
}

void setGsmWaitingString(const char* str) {
  strcpy(gsmString,str);
  pGsmString = gsmString;
  gsmTimer = millis();
}

void setOutWaitingString(const char* str) {
  strcpy(outString,str);
  pOutString = outString;
}

boolean checkGsmWaitingString() {
  if(gsm.available()) {    
    char c = gsm.read();
#ifdef DEBUG
    Serial.write(c);
#endif
    if(c == *pGsmString) {
      digitalWrite(LED,!digitalRead(LED)); // Blink the LED
      pGsmString++;
      if(*pGsmString == '\0') {
#ifdef DEBUG
        Serial.print(F("\r\nGSM Responce success: "));
        Serial.write((uint8_t*)gsmString, strlen(gsmString));
        Serial.println();
#endif
        return true;
      }
    }
  }
  if(millis() - gsmTimer > 10000) { // Only wait 10s for response    
    if(gsmState < GSM_RUNNING) {
      Serial.println("\r\nNo response from GSM module\r\nResetting...");
      gsmState = GSM_POWER_ON;
    }// else
      //gsmState = GSM_POWER_OFF;
  }
  return false;
}

boolean checkOutWaitingString() {
  if(gsm.available()) {    
    char c = gsm.read();
#ifdef DEBUG
    Serial.write(c);
#endif
    if(c == *pOutString) {
      digitalWrite(LED,!digitalRead(LED)); // Blink the LED
      pOutString++;
      if(*pOutString == '\0') {
#ifdef DEBUG
        Serial.print(F("\r\nOut Responce success: "));
        Serial.write((uint8_t*)outString, strlen(outString));
        Serial.println();
#endif
        return true;
      }
    }
  }
  return false;
}

void call(const char* num) {
  strcpy(number,num);
  callState = CALL_NUMBER;
}

void gsmActivateAutoAnswer() {
  gsm.print(F("ATS0=001\r"));
  //waitForResponse("RING"); // Use this to detect an incoming call
}

void callHangup() {
  gsm.print(F("ATH\r"));
  callState = CALL_IDLE;
}

void callAnswer() {
  gsm.print(F("ATA\r"));
}

//gsm.print(F("AT+CSQ\r")); // Check signal strength
//waitForResponse("OK");

void sendSMS(const char* num, const char* mes) {  
  strcpy(number,num);
  strcpy(message,mes);
  smsState = SMS_MODE;
}

void readSMS(const char* index) {
  gsm.print(F("AT+CMGF=1\r"));
  gsm.print(F("AT+CMGR="));
  gsm.print(index);
  gsm.print(F("\r"));
}

void gsmPowerOn() {
  digitalWrite(powerPin,LOW);
  delay(4000);
  digitalWrite(powerPin,HIGH);
  delay(2000);
}
void gsmPowerOff() {  
  digitalWrite(powerPin,LOW);
  delay(800);
  digitalWrite(powerPin,HIGH);
  delay(6000);
}
