#include <Arduino.h>
#include "./network/accesspoint.h"
#include "esp_camera.h"
#include <WiFi.h>

#include "camera/setup.h"

// ====== TEMP Wifi Details ======
const char *ssid = "Rogers 2608";
const char *password = "9955FFC361A9";
// ===============================


// On first initialization, to avoid error spamming, we use this variable in the loop() function
bool useAccessPoint = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  if (!AccessPoint::hasWifiCreds()) {
    Serial.println("No Wifi credentials found. Starting server.");
    AccessPoint::setupAccessPoint();
    useAccessPoint = true;
  } else {
    bool connected = AccessPoint::connectToWifi();
    if (!connected) {
      Serial.println("Starting Server");
      AccessPoint::setupAccessPoint();
      useAccessPoint = true;
    }
  }
  Serial.setDebugOutput(true);
  Serial.println(ESP.getEfuseMac());

  camera_config_t* config = Camera::setup();
  esp_err_t err = Camera::initalize_camera(config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // ====== TEMP WIFI Code =======
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.print("With password ");
  Serial.println(password);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  unsigned long long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(millis() - startTime > 5000){
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // ==============================
}

void loop() {
  while (!Serial) {
    ;
  }

  if (useAccessPoint) {
    AccessPoint::connectClients();
  }
}