#include <Arduino.h>
#include <WiFi.h>

static const char *DEVICEID = "1";
static const char *SSID = "LUNA-DEVICE", DEVICEID;
static const char *PASSWORD = "1234";

WiFiServer server(80);

String header;

namespace AccessPoint {
void setupAccessPoint() {

  Serial.print("Setting AP");

  WiFi.softAP(SSID, PASSWORD);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();
}

void connectClients() {
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client) {                    // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine =
        ""; // make a String to hold incoming data from the client
    while (client.connected()) { // loop while the client's connected
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n') { 
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" "
                           "content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
         
            client.println(
                "<style>html { font-family: Helvetica; display: inline-block; "
                "margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; "
                           "color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: "
                           "2px; cursor: pointer;}");
            client.println(
                ".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");

            
            client.println("</body></html>");

            client.println();
            break;
          } else { 
            currentLine = "";
          }
        } else if (c != '\r') { 
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
} // namespace AccessPoint
