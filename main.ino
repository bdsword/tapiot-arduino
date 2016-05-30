#include <SPI.h>
#include <MFRC522.h>
#include "ESP8266.h"
#include <SoftwareSerial.h>

/*********** Taps configuration ***********/
/* constants */
#define TAP_ID 1

/* variables */
int recordID;
/******************************************/


/************** ESP8266 related **************/
/* constants */
#define SSID        "BDSword-Wireless"
#define PASSWORD    "rgtfthyg159"
#define HOST_NAME   "localhost"
#define HOST_PORT   3000

/* variables */
SoftwareSerial espSerial(10, 11);
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
	
  /* Relay setup */
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  /* Sensor setup */
  pinMode(SENSOR_PIN, INPUT);
  digitalWrite(SENSOR_PIN, HIGH);
  pulseCounter = 0;
  attachInterrupt(SENSOR_INTERRUPT, increasePulseCounter, FALLING);

  /* ESP8266 setup */
  if (wifi.setOprToStation()) {
    Serial.print("to station ok\r\n");
  } else {
    Serial.print("to station err\r\n");
  }

  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  if (wifi.disableMUX()) {
    Serial.print("single ok\r\n");
  } else {
    Serial.print("single err\r\n");
  }
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
  Serial.print("read block: ");
  for (int j=0 ; j<16 ; j++){
    Serial.write (userRFID[j]);
  }

  switchRelay();
}

void sendTurnOnRequest()
{
  uint8_t buffer[300] = {0};
  if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
    Serial.print("create tcp ok\r\n");
  } else {
    Serial.print("create tcp err\r\n");
  }

  char req[100];
  sprintf(req, "POST /taps/%d/on HTTP/1.1\nHost: localhost:3000\nContent-Type: multipart/form-data; boundary=----GG\n\n----GG\nContent-Disposition: form-data; name=\"rfid\"\n\n122345678\n----GG\n\n", TAP_ID);
  wifi.send((const uint8_t*)req, strlen(req));

  uint32_t len = wifi.recv(buffer, sizeof(buffer), 10000);
  if (len > 0) {
    Serial.print("Received:[");
    for(uint32_t i = 0; i < len; i++) {
      Serial.print((char)buffer[i]);
    }
    Serial.print("]\r\n");
  }

  if (wifi.releaseTCP()) {
    Serial.print("release tcp ok\r\n");
  } else {
    Serial.print("release tcp err\r\n");
  }
}

void sendTurnOffRequest()
{
  uint8_t buffer[300] = {0};
  if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
    Serial.print("create tcp ok\r\n");
  } else {
    Serial.print("create tcp err\r\n");
  }

  char req[100];
  sprintf(req, "POST /taps/%d/on HTTP/1.1\nHost: localhost:3000\nContent-Type: multipart/form-data; boundary=----GG\n\n----GG\nContent-Disposition: form-data; name=\"rfid\"\n\n122345678\n----GG\nContent-Disposition: form-data; name=\"used\"\n\n%d\n----GG\n\n", TAP_ID, pulseCounter);
  wifi.send((const uint8_t*)req, strlen(req));

  uint32_t len = wifi.recv(buffer, sizeof(buffer), 10000);
  if (len > 0) {
    Serial.print("Received:[");
    for(uint32_t i = 0; i < len; i++) {
      Serial.print((char)buffer[i]);
    }
    Serial.print("]\r\n");
  }

  if (wifi.releaseTCP()) {
    Serial.print("release tcp ok\r\n");
  } else {
    Serial.print("release tcp err\r\n");
  }
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