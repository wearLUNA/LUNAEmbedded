#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>
#include <Preferences.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <ArduinoJson.h>

#define DEVICE_NAME "LUNA"
#define SERVICE_UUID "00000000-1111-1234-1234-111111111111"
#define CREDENTIALS_UUID "87654321-4321-4321-4321-210987654321"
#define CONTROL_UUID "87654321-4321-4321-4321-210987654322"
#define DATA_UUID "87654321-4321-4321-4321-210987654323"

namespace BLEConnector {
// on disconnect this should go back to pairing
/** Non volatle storage */
static Preferences preferences;

/** ncoming message variable */
static BLECharacteristic *pCredentialsChar;
static BLECharacteristic *pControlChar;
static BLECharacteristic *pDataChar;

/** Queue to handle incoming JSON payloads*/
static QueueHandle_t payloadQueue = nullptr;

/** Indicates LUNA is connected currently via BLE */
extern boolean connectedBt;

constexpr int32_t max_file_byte_size = (100 * 1024); //50 KB

constexpr int32_t max_file_block_byte_size = 128;

static boolean _isReading = false;
static uint8_t* _fileBuffer = nullptr;
static size_t _expectedSize = 0;
static size_t _receivedSoFar = 0;

/**
 * Struct representing JSON data.
 */
struct Payload {
  char data[256];
};

class FileControlCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *chr) override;
};

class FileDataCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *chr) override;
};

/**
 * Overriding BLE callback characteristics
 */
class CredentialsCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *chr) override;
};

/**
 * Overriding BLE server callback
 */
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override;
  void onDisconnect(BLEServer *pServer) override;
};


bool isFileTransferComplete();

uint8_t* getFileBuffer();

size_t getFileSize();



/**
 * Setup function to make ESP32 BLE server
 */
void BLESetup();

/**
 * Stores credentials persistently using Preferences
 */
void storeCredentials(const char *newSSID, const char *newPassword);

/**
 * Checks if the persistent data has WiFi credentials
 */
bool hasWifiCreds();

/**
 * Function to be used later.
 */
void clearWifiCreds();

/**
 * Uses the stored Wifi Credentials to login to wifi. Use at boot time.
 *
 * @return true if successfully connected, false if there was an error
 */
bool connectToWifi();

/**
 * Helper function for networkHandlerTask. Processes incoming JSON payload.
 * 
 * @param payload String representing incoming JSON payload
 */
static bool _processJsonPayload(const char *payload);

static bool _readData(JsonDocument &doc);

/**
 * Starts the BLE server
 */
static void _startAdvertising();

/**
 * Uses incoming credentials to login to wifi. Meant for use inside the class.
 * Use at own discretion
 *
 * @param ssid ssid to connect to
 * @param password password of network
 * @return true if successfully connected, false if there was a password error
 * or a network error
 */
static bool _connectToWifi(const String &ssid, const String &password);

/**
 * Asynchronous task using FreeRTOS to connect LUNA to WiFi
 */
static void _networkHandlerTask(void *pvParameters);

} // namespace BLEConnector

#endif // BLUETOOTH_H