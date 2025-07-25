#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>

#include "accesspoint.h"

// TODO: Make these more secure l8r. Or not. idk
static const char *AP_SSID = "LUNA-DEVICE";
static const char *AP_PASSWORD = "123412341234";

WiFiServer server(80);

Preferences preferences;

namespace AccessPoint {

void setupAccessPoint() {
  Serial.print("Setting AP");
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();
}

void storeCredentials(const char *newSSID, const char *newPassword) {
  preferences.begin("wifiCreds", false);
  preferences.putString("ssid", newSSID);
  preferences.putString("password", newPassword);
  preferences.end();
  delay(1000);
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

bool connectToWifi() {
  if (!hasWifiCreds()) {
    Serial.println("\nFailed to connect to WiFi network. No SSID or Pasword");
    return false;
  }

  preferences.begin("wifiCreds", true);
  if (!isNetworkVisible(preferences.getString("ssid"))) {
    Serial.println("\nNetwork not found. Check if the WiFi is in range.");
    return false;
  }
  
  bool connected = _connectToWifi(preferences.getString("ssid"),
                                  preferences.getString("password"));
  preferences.end();
  if (connected) {
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    // TODO find a way to figure out which option happened and handle the network error. Look into system_event_info_t. for now assume strong WiFi signals always
    Serial.println(
        "\nFailed to connect to WiFi. Check password or network try again.");
    return false;
  }
}

bool _connectToWifi(const String &ssid, const String &password) {
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 20000;

  WiFi.begin(ssid, password);
  if (millis() - startAttemptTime >= timeout) {
    while (WiFi.status() != WL_CONNECTED) {
      return false;
    }
    Serial.print(".");
    delay(100);
  }
  return true;
}


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

/**
 * Helper function for connectClients(). Processes the incoming HTTP request
 */
bool processHeader(const char *headerPtr, int *contentLength,
                   bool *isPostConnect) {
  if (strstr(headerPtr, "POST /connect") != nullptr) {
    *isPostConnect = true;
    const char *clPtr = strstr(headerPtr, "Content-Length: ");
    if (clPtr != nullptr) {
      clPtr +=
          strlen("Content-Length: "); 
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
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return false;
  }

  const char *receivedSSID = doc["ssid"];
  const char *receivedPassword = doc["password"];

  Serial.println("Parsed JSON:");
  Serial.print("SSID: ");
  Serial.println(receivedSSID);
  Serial.print("Password: ");
  Serial.println(receivedPassword);

  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 20000;

  bool connected = _connectToWifi(receivedSSID, receivedPassword);

  if (connected) {
    storeCredentials(receivedSSID, receivedPassword);
    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.print("Failed to c");
    return false;
  }
}

// TODO: Add verification. Not necessary now but never know
void connectClients() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = ""; 
    bool isPostConnect = false;
    int contentLength = 0;
    bool jsonReceived = false;
    String header = "";
    String body = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;
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

    if (isPostConnect && contentLength > 0) {
      unsigned long startTime = millis();
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
