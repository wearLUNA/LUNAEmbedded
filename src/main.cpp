#include <Arduino.h>
#include "./network/accesspoint.h"
#include "esp_camera.h"
#include <WiFi.h>

#include "camera/camera.h"
#include "api/api.h"
#include "audio/audio.h"
#include "peripherals/peripherals.h"

// ====== TEMP Wifi Details ======
const char *ssid = "Rogers 2608";
const char *password = "9955FFC361A9";
// ===============================

Haptic::HapticMotor haptic;
Audio::AudioIO audio;
API::LiveClient liveClient(audio, String(ESP.getEfuseMac()));
API::APIClient apiClient(audio, String(ESP.getEfuseMac()));

// On first initialization, to avoid error spamming, we use this variable in the loop() function
bool useAccessPoint = false;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  
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

  camera_config_t config;
  Camera::setup(&config);
  esp_err_t err = Camera::initalize_camera(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  haptic.setup();

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

bool started = false;

void loop() {
  while (!Serial) {
    ;
  }

  if (useAccessPoint) {
    AccessPoint::connectClients();
  }

  audio.loop();
  apiClient.loop();

  // Start http request using apiClient.startRequest()
  // Start live websocket using liveClient.beginLive()
  // if (Touch::is_touched() && !started) {
  //   Serial.println("touched!");
  //   started = true;

  //   liveClient.beginLive();
  // }
}