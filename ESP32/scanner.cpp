/*


#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <esp_now.h>
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include <WiFi.h>

#define MAX_DEVICES 200  // Maximum number of devices to collect
#define MESSAGE_SIZE 18
char deviceMacs[MAX_DEVICES][MESSAGE_SIZE];  
int deviceCount = 0;  // Keep track of how many devices we've found

typedef enum
{
  IDLE,
  ACTIVE,
  
} ScannerState;
ScannerState scannerState;

typedef struct struct_message 
{
    char msg[32];
} struct_message;

typedef struct struct_mac_message
{
    char mac[18];
} struct_mac_message;

struct_message receivedMessage;
uint8_t centralMAC[] = {0xA0, 0xB7, 0x65, 0x15, 0x02, 0xB8};

// Callback when data is received
void btScanCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

void setup() 
{
  scannerState = IDLE;
  Serial.begin(9600);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Ensure no connection to WiFi

  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("ESP-NOW Init Failed");
    return;  
  }

  esp_now_register_send_cb(OnSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, centralMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  if (!btStart()) {
      Serial.println("Failed to initialize Bluetooth!");
      return;
  }

  esp_bluedroid_init();
  esp_bluedroid_enable();
  esp_bt_gap_register_callback(btScanCallback);
}

unsigned long lastScanTime = 0;

void loop() {
    if (scannerState == ACTIVE && millis() - lastScanTime > 10000) { // Scan every 10 sec
        lastScanTime = millis();
        deviceCount = 0; // Reset device list before each scan
        Serial.println("🔄 Starting a new scan...");
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    }
}

///////////////////////////////////////////////////////////////////////////

void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) 
{
    memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));
    Serial.print("Received message: ");
    Serial.println(receivedMessage.msg);

    if (strcmp(receivedMessage.msg, "end") == 0)
      scannerState = IDLE;
    if (strcmp(receivedMessage.msg, "start") == 0)
      scannerState = ACTIVE;

    Serial.print("Scanner state: "); 
    Serial.println(scannerState == ACTIVE ? 1 : 0);
}

// Function to check if a MAC address already exists in the list
bool isDuplicate(const char* mac) {
    for (int i = 0; i < deviceCount; i++) {
        if (strcmp(deviceMacs[i], mac) == 0) {
            return true;  // Duplicate found
        }
    }
    return false;  // No duplicate
}

// Callback to handle Bluetooth events
void btScanCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (event == ESP_BT_GAP_DISC_RES_EVT) {
        // Collect the MAC address of the found device
        char bda[18];
        sprintf(bda, "%02X:%02X:%02X:%02X:%02X:%02X",
                param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
                param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

        // Check if the MAC address is a duplicate
        if (!isDuplicate(bda) && deviceCount < MAX_DEVICES) {
            // Store the MAC address in the array if it's not a duplicate
            strncpy(deviceMacs[deviceCount], bda, 18);
            deviceCount++;
        }
    }

    if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            // Scan has completed, print all collected MAC addresses
            Serial.println("\n📍 Scan Completed!");
            Serial.println("📋 Collected Bluetooth MAC Addresses:");
            for (int i = 0; i < deviceCount; i++) {
                Serial.println(deviceMacs[i]);
            }

            struct_mac_message message;
            for (int i = 0; i < deviceCount; i++) 
            {
                strncpy(message.mac, deviceMacs[i], (int) MESSAGE_SIZE);
                esp_err_t result = esp_now_send(centralMAC, (uint8_t *)&message, sizeof(message));
                if (result == ESP_OK) {
                    Serial.print("✅ Sent MAC: ");
                } else {
                    Serial.print("❌ Failed to send MAC: ");
                }
                Serial.println(message.mac);
            }

            strncpy(message.mac, "end", (int) MESSAGE_SIZE);
            esp_now_send(centralMAC, (uint8_t *)&message, sizeof(message));

            for(int i = 0; i< MAX_DEVICES ; i++)
               strncpy(deviceMacs[i], "" , (int) MESSAGE_SIZE);
            
        }
    }
}


*/