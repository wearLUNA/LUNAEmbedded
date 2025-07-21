#include <Arduino.h>
#include "./network/bluetooth.h"

void setup() {
  Serial.begin(115200);
  delay(5000);
  log_i("Setup Begun");
  BLEConnector::BLESetup();
}

void loop() {
}