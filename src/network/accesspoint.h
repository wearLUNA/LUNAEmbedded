#ifndef ACCESSPOINT_H
#define ACCESSPOINT_H

#include <Arduino.h>
#include <WiFi.h>

namespace AccessPoint {

// Function declarations

/**
 * Setup function to make ESP32 Access point for the handshake
 */
void setupAccessPoint();

/**
 * Setup function to expect  
 */
void connectClients();
void storeCredentials(const char* newSSID, const char* newPassword);
void clearWifiCreds();
bool hasWifiCreds();
void connectToWifi();

} // namespace AccessPoint

#endif // ACCESSPOINT_H
