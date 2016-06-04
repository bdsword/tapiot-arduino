#include <SPI.h>
#include <MFRC522.h>
#include "ESP8266.h"
#include <SoftwareSerial.h>
#include "config.h"

/*********** Taps configuration ***********/
/* variables */
int recordID = -1; // recordID = -1 : Tap is now off
/******************************************/


/************** ESP8266 related **************/
/* variables */
SoftwareSerial espSerial(7, 8);
ESP8266 wifi(espSerial);
/*********************************************/


/*************** relay related ***************/
/* constants */
#define RELAY_PIN 13

/* variables */
bool relayNowState = 0;
/*********************************************/


/************ flow sensor related ************/
/* constants */
#define SENSOR_INTERRUPT 0
#define SENSOR_PIN 2

/* variables */
volatile int pulseCounter; 

/* function declaration */
void increasePulseCounter();
/*********************************************/


/************ RFID related ************/
/* constants */
#define SS_PIN 10
#define RST_PIN 9
#define STORAGE_BLOCK_NUM 2

/* variables */
byte userRFID[18];

/* function declaration */
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
/**************************************/

void setup()
{
  Serial.begin(9600);
	
  /* RFID setup */
  SPI.begin();
  mfrc522.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
	
  /* Relay setup */
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  /* Sensor setup */
  pinMode(SENSOR_PIN, INPUT);
  digitalWrite(SENSOR_PIN, HIGH);
  pulseCounter = 0;
  attachInterrupt(SENSOR_INTERRUPT, increasePulseCounter, FALLING);

  /* ESP8266 setup */
  Serial.print("Try to chage ESP8266 to station...");
  do {
    wifi.restart();
    Serial.print('.');
  } while(!wifi.setOprToStation());
  Serial.print("\r\nTo station ok\r\n");


  Serial.print("Try to join AP...");
  do {
    Serial.print('.');
  } while(!wifi.joinAP(SSID, PASSWORD));
  Serial.print("\r\nJoin AP success\r\n");
  Serial.print("IP: ");
  Serial.println(wifi.getLocalIP().c_str());


  if (wifi.disableMUX()) {
    Serial.print("single ok\r\n");
  } else {
    Serial.print("single err\r\n");
  }

  Serial.print("Try to connected to socket server...");
  while (!connectSocketServer()) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("\r\nconnect to socket server ok!\r\n");
}

void loop() 
{
  mfrc522.PCD_Init();
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  readBlock(STORAGE_BLOCK_NUM, userRFID);
  Serial.print("RFID attached\r\n");
  if (recordID == -1) {
    if (sendTurnOnRequest((const char*)userRFID)){
      switchRelay();
    }
  } else {
    if (sendTurnOffRequest((const char*)userRFID)) {
      switchRelay();
    }
  }
  delay(2000);
}

bool connectSocketServer()
{
  uint8_t buf[10];
  Serial.print("Try to create tcp connection...");
  do {
    Serial.print('.');
  } while(!wifi.createTCP(HOST_NAME, HOST_PORT));
  Serial.print("\r\n");

  String init_req = String(TAP_ID) + '\n';
  wifi.send((const uint8_t*)init_req.c_str(), init_req.length());

  wifi.recv(buf, sizeof(buf), 10000);
  // Serial.print("Debug: " + String((const char *)buf) + ", len: " + strlen((const char *)buf));
  if (strcmp((char *)buf, "OK\n") == 0) {
    wifi.send((const uint8_t*)"OK\n", strlen("OK\n"));
    return true;
  }
  return false;
}

bool sendTurnOnRequest(const char *rfid)
{
  uint8_t buf[10] = {0};

  String req = String("1 ") + rfid + "\n";
  wifi.send((const uint8_t*)req.c_str(), req.length());
  wifi.recv(buf, sizeof(buf), 10000);
  if (buf[0] == '1') {
    String recordIdString = String((const char*)buf).substring(2, strlen((const char*)buf));
    Serial.print("Turn on tap success!");
    // Serial.print("Debug: " + recordIdString + ", len: " + recordIdString.length());
    recordID = atoi(recordIdString.c_str());
    // Serial.print("Debug2: " + recordID);
    return true;
  } else if (buf[0] == '0') {
    Serial.print("Turn on tap failed!");
    return false;
  }
  return false;
}

bool sendTurnOffRequest(const char *rfid)
{
  uint8_t buf[10] = {0};

  String req = String("0 ") + rfid + " " + recordID + " " + pulseCounter + "\n";
  wifi.send((const uint8_t*)req.c_str(), req.length());
  wifi.recv(buf, sizeof(buf), 10000);
  if (buf[0] == '1') {
    Serial.print("Turn off tap success!");
    recordID = -1;
    return true;
  } else if (buf[0] == '0') {
    Serial.print("Turn off tap failed!");
    return false;
  }
  return false;
}

void setRelayState(bool relayState) 
{
  digitalWrite(RELAY_PIN, relayState);

  Serial.print("Relay status is now: ");
  Serial.println(relayState);
}

void switchRelay()
{
  relayNowState = !relayNowState;
  setRelayState(relayNowState);
  delay(1000);
}


void increasePulseCounter()
{
  pulseCounter ++;
}

int readBlock(int blockNumber, byte arrayAddress[]) {
  int largestModulo4Number = blockNumber / 4 * 4;
  int trailerBlock = largestModulo4Number + 3;
  MFRC522::StatusCode status;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("PCD_Authenticate() failed (read): ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 3;
  }

  byte bufSize = 18;
  status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &bufSize);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_read() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 4;
  }
  delay(500);
}