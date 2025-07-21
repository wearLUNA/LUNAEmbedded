#include <Arduino.h>
#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Preferences.h>
#include <WiFi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "nvs_flash.h"

#include "bluetooth.h"

namespace BLEConnector {
boolean connectedBt = false;

void FileControlCallbacks::onWrite(BLECharacteristic *chr) {
  std::string val = chr->getValue();
  log_i(">> Received via BLE (Base64): %s\n", val.c_str());

  if (val.empty())
    return;

  Payload p;
  strlcpy(p.data, val.c_str(), sizeof(p.data));
  if (xQueueSend(payloadQueue, &p, 0) != pdTRUE) {
    log_i("QUEUE FULL! Dropping payload\n");
  } else {
    log_i("Queued %zu bytes for processing\n", val.length());
  }
}

void FileDataCallback::onWrite(BLECharacteristic *chr) {
  if (!_isReading) {
    return;
  }

  std::string chunk = chr->getValue();
  const uint8_t *ptr = (const uint8_t *)chunk.data();
  size_t len = chunk.length();
  size_t toCopy = len;
  if (_receivedSoFar + len > _expectedSize) {
    toCopy = _expectedSize - _receivedSoFar;
  }

  memcpy(_fileBuffer + _receivedSoFar, ptr, toCopy);
  _receivedSoFar += toCopy;

  if (_receivedSoFar >= _expectedSize) {
    _isReading = false;
    Serial.printf("File transfer complete: %u bytes received\n",
                  (unsigned)_receivedSoFar);
  }
}

void CredentialsCallbacks::onWrite(BLECharacteristic *chr) {
  std::string val = chr->getValue();
  log_i(">> Received via BLE (Base64): %s\n", val.c_str());
  size_t len = val.length();
  if (len >= sizeof(Payload::data)) {
    log_i("Incoming JSON too large for Payload buffer (%d >= %d)", len,
      sizeof(Payload::data));
      return;
    }

  _processJsonPayload(val.c_str());
}

void ServerCallbacks::onConnect(BLEServer *pServer) {
  connectedBt = true;
  log_i("Client connected");
}

void ServerCallbacks::onDisconnect(BLEServer *pServer) {
  connectedBt = false;
  log_i("Client disconnected");
  _startAdvertising();
}

void BLESetup() {
  Serial.println("Starting BLE setup...");

  // Initialize NVS — needed for BLE
  //   nvs_flash_erase();
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_init());
  }

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  esp_bluedroid_init();
  esp_bluedroid_enable();

  BLEDevice::init(DEVICE_NAME);

  BLEDevice::setMTU(512);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCredentialsChar = pService->createCharacteristic(
      CREDENTIALS_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pControlChar = pService->createCharacteristic(
      CONTROL_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pDataChar = pService->createCharacteristic(
      DATA_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  pCredentialsChar->addDescriptor(new BLE2902());
  pControlChar->addDescriptor(new BLE2902());
  pDataChar->addDescriptor(new BLE2902());
  pControlChar->setCallbacks(new CredentialsCallbacks());
  pCredentialsChar->setCallbacks(new FileControlCallbacks());
  pDataChar->setCallbacks(new FileDataCallback());

  pService->start();

  _startAdvertising();

  payloadQueue = xQueueCreate(4, sizeof(Payload));
  if (payloadQueue == nullptr) {
    log_i("Failed to create BLE payload queue");
    return;
  }

  xTaskCreatePinnedToCore(_networkHandlerTask,  // Task function
                          "NetworkHandlerTask", // Task name
                          4096,                 // Stack size in words
                          nullptr,              // Parameters
                          1,                    // Priority
                          nullptr,              // Task handle
                          1                     // Core (BLE runs on 0)
  );

  Serial.println("BLE Service started. Registered Characteristics:");
  Serial.println(pCredentialsChar->getUUID().toString().c_str());
  Serial.println(pControlChar->getUUID().toString().c_str());
  Serial.println(pDataChar->getUUID().toString().c_str());
}

static bool _processJsonPayload(const char *payload) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    log_i("deserializeJson() failed: %s", error.f_str());
    return false;
  }

  const char *type = doc["type"];
  if (!type) {
    log_i("no type key");
    return false;
  }
  log_i("before\n");
  if (strcmp(doc["type"], "networkconnect") == 0) {
    const char *receivedSSID = doc["ssid"] | "";
    const char *receivedPwrd = doc["pwd"] | "";

    log_i("Parsed JSON: SSID: %s  Password: %s", receivedSSID, receivedPwrd);

    unsigned long startAttemptTime = millis();
    const unsigned long timeout = 20000;

    bool connected = _connectToWifi(receivedSSID, receivedPwrd);
    if (connected) {
      storeCredentials(receivedSSID, receivedPwrd);
      log_i("\nConnected to the WiFi network. Local ESP32 IP: %s",
            WiFi.localIP());
      return true;
    } else {
      log_i("Failed to connect");
      return false;
    }
  } else if (strcmp(doc["type"], "filetransfer") == 0) {
    return _readData(doc);
  } else {
    log_i("Error, JSON format is incorrect or incompatible");
    return false;
  }
}

static bool _readData(JsonDocument &doc) {
  if (_isReading) {
    log_i("Reader already busy");
    return false;
  }

  const char *cmd = doc["cmd"]; 
  uint32_t sz = doc["size"];    

  if (strcmp(doc["cmd"], "start") != 0) {
    log_i("Control JSON missing 'start' or invalid size"); 
    return false;
  }
  _fileBuffer = (uint8_t *)malloc(sz);

  _expectedSize = sz;
  _receivedSoFar = 0;
  _isReading = true;
  log_i("started file transfer: expecting %u bytes\n", sz);
  return true;
}

static void _startAdvertising() {
  BLEAdvertising *pAdv = BLEDevice::getAdvertising();

  BLEAdvertisementData advData;
  advData.setName(DEVICE_NAME);
  advData.setCompleteServices(BLEUUID(SERVICE_UUID));
  pAdv->setAdvertisementData(advData);

  pAdv->setScanResponse(true);

  // iPhone workaround
  pAdv->setMinPreferred(0x06);
  pAdv->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

  Serial.println(">> BLE Advertising as \"" DEVICE_NAME
                 "\" with service " SERVICE_UUID);
}

void _networkHandlerTask(void *pvParameters) {
  Payload p;

  while (true) {
    if (xQueueReceive(payloadQueue, &p, portMAX_DELAY) == pdTRUE) {
      log_i("Processing payload in background");

      _processJsonPayload(p.data);
    }
  }
}

void storeCredentials(const char *newSSID, const char *newPassword) {
  preferences.begin("wifiCreds", false);
  preferences.putString("ssid", newSSID);
  preferences.putString("password", newPassword);
  preferences.end();
  Serial.println("Credentials stored persistently.");
}

bool hasWifiCreds() {
  preferences.begin("wifiCreds", false);
  String storedSSID = preferences.getString("ssid", "");
  preferences.end();

  return storedSSID != "";
}

// TODO: use this somewhere. Add a button to reset or something
void clearWifiCreds() {
  preferences.begin("wifiCreds", false);
  preferences.putString("ssid", "");
  preferences.putString("password", "");
  preferences.end();
  Serial.println("WiFi credentials cleared.");
}

/**
 * Helper function for connectToWifi. Searches all available networks for
 * given ssid.
 *
 * @param targetSSID passed by reference
 */
bool isNetworkVisible(const String &targetSSID) {
  Serial.println("Scanning for available networks...");
  int numNetworks = WiFi.scanNetworks();
  Serial.println("Scan complete.");

  for (int i = 0; i < numNetworks; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid == targetSSID) {
      return true;
    }
  }
  return false;
}

bool connectToWifi() {
  if (!hasWifiCreds()) {
    Serial.println("\nFailed to connect to WiFi network. No SSID or Pasword");
    return false;
  }

  preferences.begin("wifiCreds", true);
  String ssid = preferences.getString("ssid");
  String pass = preferences.getString("password");
  preferences.end();

  if (!isNetworkVisible(ssid)) {
    Serial.println("\nNetwork not found. Check if the WiFi is in range.");
    return false;
  }

  bool connected = _connectToWifi(ssid, pass);
  if (connected) {
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    // TODO find a way to figure out which option happened and handle the
    // network error. Look into system_event_info_t. for now assume strong WiFi
    // signals always
    Serial.println(
        "\nFailed to connect to WiFi. Check password or network try again.");
    WiFi.disconnect(true);
    return false;
  }
}

static bool _connectToWifi(const String &ssid, const String &password) {
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 20000;
  WiFi.disconnect(true);
  log_i("Connecting to SSID: %s with password: %s", ssid.c_str(),
        password.c_str());
  WiFi.setAutoReconnect(false); 
  WiFi.persistent(false);       
  delay(100);

  log_i("Connecting to SSID: %s\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  bool success = WiFi.status() == WL_CONNECTED;

  if (!success) {
    WiFi.disconnect(true);
    log_i("Connection attempt timed out");
  }

  return success;
}

} // namespace BLEConnector