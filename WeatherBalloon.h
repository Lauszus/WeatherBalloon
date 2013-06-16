#ifndef _weatherBalloon_h_
#define _weatherBalloon_h_

#define DEBUG

#define GSM_POWER_ON              0
#define GSM_POWER_ON_WAIT         1
#define GSM_SET_PIN               2
#define GSM_CHECK_CONNECTION      3
#define GSM_CHECK_CONNECTION_WAIT 5
#define GSM_CONNECTION_RESPONSE   6
#define GSM_RUNNING               7

#define GSM_POWER_OFF             8
#define GSM_POWER_OFF_WAIT        9

#define SMS_IDLE                  0
#define SMS_MODE                  1
#define SMS_ALPHABET              2
#define SMS_NUMBER                3
#define SMS_CONTENT               4
#define SMS_WAIT                  5

#define CALL_IDLE                 0
#define CALL_NUMBER               1
#define CALL_SETUP                2
#define CALL_SETUP_WAIT           3
#define CALL_RESPONSE             4
#define CALL_ACTIVE               5

const uint8_t powerPin = 4;
const uint8_t LED = 13;

char gsmString[20], outString[20];
char *pGsmString = gsmString, *pOutString = outString;

char number[11];
char message[161];

char receiveSmsString[] = "+CMTI: \"SM\","; // +CMTI: "SM",index\r\n
char *pReceiveSmsString = receiveSmsString;
bool readIndex = false;
char lastIndex[5];
uint8_t indexCounter;

uint8_t gsmState, smsState, callState;
uint32_t gsmTimer;

#endif
