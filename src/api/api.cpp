#include "api.h"

namespace API {

  const char *SERVER_URL = "luna-backend-864401919642.us-east1.run.app";

  APIClient::APIClient(Audio::AudioIO& audioRef, String authToken, const char *serverUrl) : audio(audioRef) {
    this->authToken = authToken;
    this->serverUrl = serverUrl;
    
    #if USE_SECURE == true
      client.setCACert(ROOT_CA);
    #else
      client.setInsecure();
    #endif
  }

  bool APIClient::startRequest() {
    if (WiFi.status() == WL_CONNECTED && !processing) {
      Serial.println("Sending HTTP request");

      processing = true;
      http.begin(client, "https://" + String(serverUrl) + "/describe-stream");
      http.setTimeout(HTTP_TIMEOUT);

      http.addHeader("Content-Type", "image/jpeg");
      http.addHeader("Authorization", authToken);

      const char *headerKeys[] = {"Content-Length"};
      const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
      http.collectHeaders(headerKeys, headerKeysCount);

      camera_fb_t *fb = Camera::take_photo();
      
      int response = http.POST(fb->buf, fb->len);

      if (response == 200) {
        Serial.println("Response received");
        
        ptr = http.getStreamPtr();
        contentLength = http.header("Content-Length").toInt();
        
        Serial.printf("Content length: %d\n", contentLength);

        return true;
      }
    }

    processing = false;
    return false;
  }

  void APIClient::loop() {
    if (processing) {
      int bufferCap = 0;

      while (bufferCap < TEMP_BUF_LEN && bytesCollected < contentLength) {
        int readable = ptr->available();
        if (readable) {
          int toRead = min(TEMP_BUF_LEN - bufferCap, readable);
          int read = ptr->read(&temp_buf[bufferCap], toRead);

          bufferCap += read;
          bytesCollected += read;
        }
      }
      
      Serial.printf("Bytes Collected: %d\n", bytesCollected);

      audio.write(temp_buf, TEMP_BUF_LEN);

      if (bytesCollected >= contentLength) {
        processing = false;
        bytesCollected = 0;
        contentLength = 0;
        ptr = nullptr;
        http.end();
      }
    }
  }

  LiveClient::LiveClient(Audio::AudioIO& audioRef, String authToken, const char *serverUrl) : audio(audioRef) {
    this->authToken = authToken;
    this->serverUrl = serverUrl;
  }

  void LiveClient::websocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
      case WStype_DISCONNECTED:
        // cleanup and end input send task
        Serial.println("Socket Disconnected");
        endLive();
        break;
      case WStype_CONNECTED:
        // indicate socket has been connected somehow
        // maybe vibrate haptic?
        Serial.println("Socket Connected");
        break;
      case WStype_BIN:
        // we got audio data back in binary
        // write to audio
        Serial.println("got audio");
        audio.write(payload, length);
        break;
      case WStype_TEXT:
        // We shouldn't be getting back text data
        break;
      case WStype_ERROR:
        // something was wrong
        // disconnect  and end send task
        Serial.println("Error Occured. Disconnecting Socket.");
        endLive();
        break;
      default:
        break;
    }
  }

  bool LiveClient::beginLive() {
    if (!processing) {
      Serial.println("Beginning connection");

      processing = true;

      #if USE_SECURE == true
        webSocket.beginSslWithCA(serverUrl, 443, "/", ROOT_CA, "");
      #else
        webSocket.begin(server_url, 80, "/");
      #endif

      webSocket.onEvent(websocketEventWrapper, this);
      webSocket.setAuthorization(authToken.c_str());
      webSocket.enableHeartbeat(1000, 3000, 3);
      webSocket.setReconnectInterval(1000);

      xTaskCreatePinnedToCore(
        websocketTaskWrapper,
        "WebSocketLoop",
        6144,
        this,
        2,
        &wsTaskHandle,
        1
      );

      return true;
    }
    return false;
  }

  void LiveClient::endLive() {
    processing = false;
    
    // webSocket.disconnect();
  }

  void LiveClient::websocketEventWrapper(WStype_t type, uint8_t * payload, size_t length, void* ptr) {
    LiveClient* instance = static_cast<LiveClient*>(ptr);
    Serial.println("Event received");
    Serial.println(instance->processing);
    instance->websocketEvent(type, payload, length);
  }

  void LiveClient::websocketTask() {
    while (!webSocket.isConnected()) {
      if(!processing) break;
      webSocket.loop();
    }
    
    auto connectionStart = millis();
    int loopNum = 0;

    while (millis() - connectionStart < MAX_WEBSOCKET_TIME && processing) {
      webSocket.loop();

      if (loopNum > 75) {
        camera_fb_t *fb = Camera::take_photo();
        String image = base64::encode(fb->buf, fb->len);
        String imageReq = "{ \"mimeType\": \"image/jpeg\", \"data\": \"" + image + "\"}";

        webSocket.sendTXT(imageReq.c_str());

        Camera::return_fb(fb);
        loopNum = 0;
      }
      // TODO: get mic data, transform into base64 and send to websocket
      audio.read();
      String encoded = base64::encode(audio.bufferIn, IN_BUFFER_SIZE);
      String request = "{ \"mimeType\": \"audio/pcm\", \"data\": \"" + encoded + "\"}";
      
      webSocket.sendTXT(request.c_str());

      loopNum++;
    }
    
    audio.flushSpeakerBuffer();
    processing = false;
    webSocket.disconnect();
    vTaskDelete(NULL);
  }

  void LiveClient::websocketTaskWrapper(void* ptr) {
    LiveClient* instance = static_cast<LiveClient*>(ptr);
    instance->websocketTask();
  }

  bool LiveClient::isConnected() {
    return webSocket.isConnected();
  }
}