#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>

#include "accesspoint.h"

// TODO: Make these more secure l8r. Or not. idk
static const char *AP_SSID = "LUNA-DEVICE";
static const char *AP_PASSWORD = "123412341234";

// Server to host the Access Point
WiFiServer server(80);

// For persistent data
Preferences preferences;

namespace AccessPoint {

/**
 * Starts the WiFi Server at port 80 and sets the appropriate SSID and Password
 */
void setupAccessPoint() {
  Serial.print("Setting AP");
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();
}

/**
 * Stores credentials persistently using Preferences
 */
void storeCredentials(const char *newSSID, const char *newPassword) {
  preferences.begin("wifiCreds", false);
  preferences.putString("ssid", newSSID);
  preferences.putString("password", newPassword);
  preferences.end();
  Serial.println("Credentials stored persistently.");
}

/**
 * Checks if the persistent data has WiFi credentials
 */
bool hasWifiCreds() {
  preferences.begin("wifiCreds", false);
  String storedSSID = preferences.getString("ssid", "");
  preferences.end();

  return storedSSID != "";
}

/**
 * Function to be used later.
 */
// TODO: use this somewhere. Add a button to reset or something
void clearWifiCreds() {
  preferences.begin("wifiCreds", false);
  preferences.putString("ssid", "");
  preferences.putString("password", "");
  preferences.end();
  Serial.println("WiFi credentials cleared.");
}

/**
 * Uses the stored Wifi Credentials to login to wifi.
 */
void connectToWifi() {
  if (hasWifiCreds()) {
    preferences.begin("wifiCreds", true);
    WiFi.begin(preferences.getString("ssid"),
               preferences.getString("password"));
    Serial.println("\nConnecting");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
    return;
  }
  Serial.println("\nFailed to connect to WiFi network. No SSID or Pasword");
}

/**
 * Helper function for connectClients(). Processes the incoming HTTP request
 */
bool processHeader(const char *headerPtr, int *contentLength,
                   bool *isPostConnect) {
  // Check for "POST /connect" in the header.
  if (strstr(headerPtr, "POST /connect") != nullptr) {
    *isPostConnect = true;
    // Find "Content-Length: " within the header.
    const char *clPtr = strstr(headerPtr, "Content-Length: ");
    if (clPtr != nullptr) {
      clPtr +=
          strlen("Content-Length: "); // move pointer past header field name
      // atoi will convert the number until a non-digit is found.
      *contentLength = atoi(clPtr);
    }
  } else {
    *isPostConnect = false;
  }
  return true;
}

/**
 * Helper function for connectClients(). Processes incoming JSON payload
 */
bool processJsonPayload(const char *payload) {
  // Create a JSON document with enough capacity for your payload.
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return false;
  }

  // Extract the SSID and password from the JSON.
  const char *receivedSSID = doc["ssid"];
  const char *receivedPassword = doc["password"];

  Serial.println("Parsed JSON:");
  Serial.print("SSID: ");
  Serial.println(receivedSSID);
  Serial.print("Password: ");
  Serial.println(receivedPassword);

  // Store the credentials permanently.
  storeCredentials(receivedSSID, receivedPassword);

  // Optionally connect to the new WiFi network immediately.
  WiFi.begin(receivedSSID, receivedPassword);
  Serial.println("\nConnecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  return true;
}

/**
 * Waits until a client makes a request.
 */
// TODO: Add verification. Not necessary now but never know
void connectClients() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = ""; // To hold incoming data from the client
    bool isPostConnect = false;
    int contentLength = 0;
    bool jsonReceived = false;
    String header = "";
    String body = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;
        // Detect end of header (a blank line)
        if (c == '\n') {
          if (currentLine.length() == 0) {
            processHeader(header.c_str(), &contentLength, &isPostConnect);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') { // A carriage return should not be added. Its
                                // the second last character
          currentLine += c;
        }
      }
    }

    // If it was a POST request, try to read the JSON body
    if (isPostConnect && contentLength > 0) {
      unsigned long startTime = millis();
      // Wait for the body to become available
      while (client.available() < contentLength &&
             (millis() - startTime) < 2000) {
        delay(10);
      }
      while (client.available()) {
        char c = client.read();
        body += c;
      }
      Serial.println("Received JSON body: ");
      Serial.println(body);
      jsonReceived = true;
    }

    // If we received JSON, parse it using ArduinoJson
    if (jsonReceived) {
      processJsonPayload(body.c_str());

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println(
          "<!DOCTYPE html><html><body><h1>ESP32 Response</h1></body></html>");
    } else {
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println("<!DOCTYPE html><html><body><h1>JSON Not "
                     "Received</h1></body></html>");
    }
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

} // namespace AccessPoint
