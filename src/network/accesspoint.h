#ifndef ACCESSPOINT_H
#define ACCESSPOINT_H

#include <Arduino.h>
#include <WiFi.h>

namespace AccessPoint {

// TODO add conditional debugging based on Debug mode

// Function declarations
/**
 * Setup function to make ESP32 Access point for the handshake
 */
void setupAccessPoint();

/**
 * Stores credentials persistently using Preferences
 */
void storeCredentials(const char* newSSID, const char* newPassword);

/**
 * Checks if the persistent data has WiFi credentials
 */
bool hasWifiCreds();

/**
 * Waits until a client makes a request. Takes in the JSON with SSID and password and connects ESP to wifi
 */
void connectClients();

/**
 * Function to be used later.
 */
void clearWifiCreds();

/**
 * Uses the stored Wifi Credentials to login to wifi.
 *
 * @return true if successfully connected, false if there was an error
 */
bool connectToWifi();

/**
 * Uses incoming credentials to login to wifi. Meant for use inside the class. Use at own discretion
 * 
 * @param ssid ssid to connect to
 * @param password password of network
 * @return true if successfully connected, false if there was a password error
 * or a network error
 */
bool _connectToWifi(const String& ssid, const String& password);

/**
 * Scans visible networks for expect ssid
 * 
 * @param targetSSID ssid to search for
 * @return true if targetSSID was found, false if it was not found
 */
bool isNetworkVisible(const String& ssidTarget);

} // namespace AccessPoint

#endif // ACCESSPOINT_H
