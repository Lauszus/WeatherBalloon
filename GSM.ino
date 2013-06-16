SoftwareSerial gsm(2, 3); // RX, TX

// The GSM module is a SIM300

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
    incomingChar = gsm.read();
#ifdef DEBUG
    if(incomingChar != -1)
      Serial.write(incomingChar);
#endif
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
        if(incomingChar != -1) {
          Serial.print(F("Connection response: "));
          Serial.println(incomingChar);
          if(incomingChar != '1') {
            delay(1000);
            gsmState = GSM_CHECK_CONNECTION;
          } else {
            Serial.println(F("GSM module is up and running!\r\n"));
            gsmState = GSM_RUNNING;
          }
        }
        break;
        
      case GSM_RUNNING:
        updateSMS();
        updateCall();
        if(incomingChar != -1) {
          if (checkSMS()) // Return true if a new SMS is received
            newSMS = true;
          checkIncomingCall();
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

boolean checkSMS() {
  if (readIndex) {
    if (incomingChar == '\r') { // End of index
      readIndex = false;
      Serial.print(F("Received SMS at index: "));
      Serial.println(lastIndex);
      return true;
    } else if (indexCounter < sizeof(lastIndex)/sizeof(lastIndex[0])-1) {
      //Serial.println(F("Storing index"));
      lastIndex[indexCounter] = incomingChar;
      lastIndex[indexCounter+1] = '\0';
      indexCounter++;
    } else {
      Serial.println(F("Index is too long"));
      readIndex = false;
    }
  } 
  // TODO: Can be put in function below when finished
  else if (incomingChar == *pReceiveSmsString) {
    pReceiveSmsString++;
    if (*pReceiveSmsString == '\0') {
      readIndex = true;
      indexCounter = 0;
      //Serial.println(F("Received SMS"));
    }
  } else
    pReceiveSmsString = receiveSmsString; // Reset pointer to start of string
  return false;
}

// TODO: Make more general function to check any string
void checkIncomingCall() {
  if (incomingChar == *pIncomingCallString) {
    pIncomingCallString++;
    if (*pIncomingCallString == '\0') {
      Serial.println(F("Incoming Call"));
      callAnswer();
    }
  } else
    pIncomingCallString = incomingCallString; // Reset pointer to start of string
}

void checkCallHangup() {
  if (incomingChar == *pCallHangupString) {
    pCallHangupString++;
    if (*pCallHangupString == '\0') {
      Serial.println(F("Call hangup!"));
      callState = CALL_IDLE;
    }
  } else
    pCallHangupString = callHangupString; // Reset pointer to start of string
}

// TODO: Check if updateSMS or updateCall is caught in a loop
void updateSMS() {
  switch(smsState) {
    case SMS_IDLE:
      break;
      
    case SMS_MODE:
      Serial.println(F("SMS setting text mode"));
      
      gsm.print(F("AT+CMGF=1\r")); // Set SMS type to text mode
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
        Serial.println(numberBuffer);
        
        gsm.print(F("AT+CMGS=\""));
        gsm.print(numberBuffer);
        gsm.print(F("\"\r"));
        setOutWaitingString(">");
        smsState = SMS_CONTENT;
      }
      break;
      
    case SMS_CONTENT:
      if(checkOutWaitingString()) {
        Serial.print(F("Message: \""));
        Serial.print(messageBuffer);
        Serial.println("\"");
        
        gsm.print(messageBuffer);
        gsm.write(26); // CTRL-Z
        setOutWaitingString("OK");
        smsState = SMS_WAIT;
      }
      break;
      
    case SMS_WAIT:
      if(checkOutWaitingString()) {
        Serial.println(F("SMS is sent"));
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
      Serial.println(numberBuffer);

      gsm.print(F("ATD")); // Dial
      gsm.print(numberBuffer);
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
      if(incomingChar != -1) {
        Serial.print(F("Connection response: "));
        Serial.println(incomingChar);
        if(incomingChar != '0') {
          delay(1000);
          callState = CALL_SETUP;
        } else {
          Serial.print(F("Call active"));
          callState = CALL_ACTIVE;
        }
      }
      break;
      
    case CALL_ACTIVE:
      if(incomingChar != -1)
        checkCallHangup();
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

// TODO: Combine these two function
boolean checkGsmWaitingString() {
  if(incomingChar != -1) {
    if(incomingChar == *pGsmString) {
      digitalWrite(LED,!digitalRead(LED)); // Blink the LED
      pGsmString++;
      if(*pGsmString == '\0') {
#ifdef DEBUG
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
      Serial.println("\r\nNo response from GSM module\r\nResetting...");
      gsmState = GSM_POWER_ON;
    }
  }
  return false;
}

boolean checkOutWaitingString() {
  if(incomingChar != -1) {
    if(incomingChar == *pOutString) {
      digitalWrite(LED,!digitalRead(LED)); // Blink the LED
      pOutString++;
      if(*pOutString == '\0') {
#ifdef DEBUG
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

void call(const char* num) {
  strcpy(numberBuffer,num);
  callState = CALL_NUMBER;
}

void gsmActivateAutoAnswer() {
  gsm.print(F("ATS0=001\r")); // 'RING' will be received on an incoming call  
}

void callHangup() {
  gsm.print(F("ATH\r")); // Response: 'OK'
  callState = CALL_IDLE;
}

void callAnswer() {
  gsm.print(F("ATA\r"));
  callState = CALL_ACTIVE;
}

//gsm.print(F("AT+CSQ\r")); // Check signal strength - response: 'OK' and then the information

void sendSMS(const char* num, const char* mes) {  
  strcpy(numberBuffer,num);
  strcpy(messageBuffer,mes);
  smsState = SMS_MODE;
}

boolean readSMS(const char* index) {
  newSMS = false;
  gsm.print(F("AT+CMGF=1\r"));
  gsm.print(F("AT+CMGR="));
  gsm.print(index);
  gsm.print(F("\r"));
  
  uint32_t startTime = millis();
  int8_t i = -1;
  boolean numberFound = false;
  boolean reading = false;
  
  // Read the senders number. The SMS is returned as:
  // +CMGR: "REC UNREAD","number",,"date"
  // content
  
  while(millis() - startTime < 1000) { // Only do this for 1s
    while(!gsm.available());
    char c = gsm.read();
#ifdef DEBUG
    Serial.write(c);
#endif
    if (reading) {
      if (i > -1) {
        if (c == '"') { // Second "
          Serial.print(F("Extracted the following number: "));
          Serial.println(numberBuffer);
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
    while(!gsm.available());
    char c = gsm.read();
#ifdef DEBUG
    Serial.write(c);
#endif
    if (reading) {
      // TODO: Gem i messageBuffer
      Serial.write(c);
      if (c == '\n') // End of second string
        break;
    }
    else if (c == '\n') { // End of first string
      reading = true;
      Serial.print(F("Received message: "));
    }
  }
  // TODO: 'return numberFound && messageFound;'
  return numberFound;
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
