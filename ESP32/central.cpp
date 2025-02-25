/*


#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#include "BluetoothSerial.h"
#include "BluetoothSerial.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>

///////////////////////////////////////////////////////////////////////////////////////
//state machine
typedef enum
{
  IDLE,
  ACTIVE,
  REPORTING

}State;

State state;


//card reader
#define PN532_SCK  18
#define PN532_MISO 19
#define PN532_MOSI 23
#define PN532_SS   5
#define SDA_PIN 21
#define SCL_PIN 22
#define LED_PIN 2

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
unsigned long lastCardScanTime = 0;
unsigned long cardScanInterval = 1000;




//adress buffer
#define MAX_DEVICES 200  
#define MESSAGE_SIZE 18

char deviceMacs[MAX_DEVICES][MESSAGE_SIZE]; 
static int addressesStored = 0;




//connection specifications
typedef struct struct_message 
{
    char msg[32];

} struct_message;

struct_message message;
struct_message receivedMessage;


const char* ssid = ".";//"894B47";
const char* password = "13456789";//"3vz98wcit9";
const char* serverNameAddStudent = "http://52.158.40.190/add_student";  
const char* serverNameAddPresence = "http://52.158.40.190/add_presence"; 

uint8_t scannerMAC[] = {0x7C, 0x9E, 0xBD, 0x65, 0x7D, 0xF4};







//function declarations
void sendAttendance(String macAddress);
void sendPostRequest(const char* url, String jsonData);
void StartScanner( );
void StopScanner( );
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len); 
void SwitchToWifi();
void SwitchToESPNow();
void SendAdresses();
void ClearAdressBuffer();

////////////////////////////////////////////////////////////////////////////////
void setup(void) 
{
  pinMode(LED_PIN,OUTPUT);

  Serial.begin(9600);
  Serial.println("");
  Serial.println("---------------------------");
  Serial.println("Power on!");

  nfc.begin();
  state = IDLE;

  WiFi.mode(WIFI_STA);

  SwitchToESPNow();


  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) 
  {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
 

  Serial.print("Found chip PN5"); Serial.print((versiondata>>24) & 0xFF, HEX);
  Serial.print(" ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);


  Serial.println("Waiting for an ISO14443A Card ...");
  Serial.println("---------------------------");
  
}

void loop(void) 
{
  if(state == ACTIVE || state == REPORTING)
  {
    digitalWrite(LED_PIN,HIGH);
  }
  else
  {
    digitalWrite(LED_PIN,LOW);
  }

  if(state == REPORTING)
  {
    Serial.println("");
    Serial.println("[Sending MAC adresses to server]");

    
    SwitchToWifi();
    SendAdresses();

    SwitchToESPNow();
    ClearAdressBuffer();
  
    state = ACTIVE;
  }

  if (millis() - lastCardScanTime >= cardScanInterval) 
  {
    lastCardScanTime = millis();

  uint8_t cardScanSuccess;
  uint8_t cardUid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  cardScanSuccess = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, cardUid, &uidLength , 100);

  if (cardScanSuccess) 
  {
    // Display some basic information about the card
    Serial.print("Found an ISO14443A card!");
    
    Serial.print("  UID Length: "); 
    Serial.print(uidLength, DEC); 
    Serial.println(" bytes");
    
    Serial.print("  UID Value: ");
    nfc.PrintHex(cardUid, uidLength);
    Serial.println("");


  switch (state)
  {
      case IDLE:
        Serial.println("Opening room , starting class!");
        Serial.println("---------------------------");

        StartScanner();
        state = ACTIVE;
        break;
  

      case ACTIVE:
        Serial.println("Locking room , ending class!");
        Serial.println("---------------------------");

        StopScanner();
        state = IDLE;
        break;

      default:
        break;
  }
 
  }
}
}

///////////////////////////////////////////////////////////////////////////////

void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
    Serial.print("Message Send Status: ");
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Success ]" : "Fail ]");
    Serial.println("");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) 
{
    memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));
    Serial.print("->: ");
    Serial.println(receivedMessage.msg);

    if (strcmp(receivedMessage.msg, "end") == 0 && state != REPORTING)
    {
      *(&state) = REPORTING;
    }
    else
    {
      strncpy(deviceMacs[addressesStored],receivedMessage.msg,MESSAGE_SIZE);
      addressesStored++;
    }


}

void StartScanner()
{
  Serial.print("[Starting scanner! ");
  strcpy(message.msg, "start");
  esp_now_send(scannerMAC, (uint8_t *)&message, sizeof(message));

}

void StopScanner()
{
  Serial.println("Stopping scanner!");
  Serial.println("----------------------------\n");
  strcpy(message.msg, "end");
  esp_now_send(scannerMAC, (uint8_t *)&message, sizeof(message));

}

void sendPostRequest(const char* url, String jsonData) 
{
  HTTPClient http;
  http.begin(url);  // Specify the URL
  http.addHeader("Content-Type", "application/json");  // Set the content type to JSON

  int httpResponseCode = http.POST(jsonData);  // Send POST request with JSON data

  // Handle the HTTP response
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
    Serial.println("Request successful!");
    Serial.println(".....................");
  } else {
    Serial.print("Failed to send request. HTTP Response Code: ");
    Serial.println(httpResponseCode);
    Serial.println("Error sending request.");
    Serial.println(".....................");

  }

  // End the HTTP request
  http.end();
}

void sendAttendance(String macAddress) {
  String classNo = "1";
  
  String jsonData = "{\"mac_address\": \"" + macAddress + "\", \"class_id\": \"" + classNo + "\"}";
  sendPostRequest(serverNameAddPresence, jsonData);
}

void SwitchToESPNow()
{
  Serial.println("");
  Serial.println("[Connecting with ESPNOW]");
  WiFi.disconnect();  // Ensure no connection to WiFi

  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("ESP-NOW Init Failed");
    return;  
  }

  esp_now_register_send_cb(OnSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, scannerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add peer");
    return;
  }
}

void SwitchToWifi()
{
  Serial.println("Attempting to connect to Wi-Fi...");
  esp_now_deinit();
  delay(200);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for the Wi-Fi to connect
  int attemptCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    attemptCount++;
    Serial.print("-> Attempt #");
    Serial.println(attemptCount);
  }

  // Successfully connected to Wi-Fi
  Serial.print("Connected to Wi-Fi! ");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void SendAdresses()
{
  for(int i = 0; i< addressesStored ; i++)
    sendAttendance(deviceMacs[i]);
}

void ClearAdressBuffer()
{
  for(int i = 0; i< addressesStored ; i++)
      strncpy(deviceMacs[i], "" , (int) MESSAGE_SIZE);

  addressesStored = 0;
}

*/