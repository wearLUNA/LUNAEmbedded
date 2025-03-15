#include <Arduino.h>
#include "./network/accesspoint.h"

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
    AccessPoint::connectToWifi();
  }
}

void loop() {
  while (!Serial) {
    ;
  }

  if (useAccessPoint) {
    AccessPoint::connectClients();
  }
}