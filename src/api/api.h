#pragma once
#include "esp_camera.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "audio/audio.h"
#include "WebSocketsClient.h"
#include <base64.h>
#include "camera/camera.h"
#include "certs.h"

namespace API {

  #define USE_SECURE true                 // Use TLS if true
  #define MAX_WEBSOCKET_TIME 120000       // Maximum duration for websocket connection (ms)
  #define HTTP_TIMEOUT 10000              // Max wait duration for HTTP response
  #define TEMP_BUF_LEN 1024               // Length of temporary buffer to hold http responses (bytes)

  extern const char *SERVER_URL;          // SERVER URL defined in api.cpp

  /**
   * @brief Client for making HTTP requests to backend
   * 
   * This client makes requests to the "/describe-stream" endpoint
   */
  class APIClient {
    private:  
      Audio::AudioIO& audio;              // Reference to AudioIO object
      HTTPClient http;            
      WiFiClientSecure client;
      WiFiClient *ptr = nullptr;          // Pointer to received HTTP response payload
      String authToken;                  // Unique authentication token for this device - EfuseMac
      const char *serverUrl;             // server url to request - default is SERVER_URL
      uint8_t temp_buf[TEMP_BUF_LEN];     // Temporary buffer to hold http response payload

      int contentLength = 0;             // length of the received paylod
      int bytesCollected = 0;            // number of bytes read from the payload
    
    public:
      bool processing = false;            // True if there is an ongoing http request going on (waiting for response)

      /**
       * @brief Construct a new APIClient object
       * 
       * The client requires an reference to the AudioIO object to send received data to speaker.
       * 
       * @param audioRef reference to the AudioIO object
       * @param authToken unique device auth token
       * @param serverUrl server url to request - default is SERVER_URL
       */
      APIClient(Audio::AudioIO& audioRef, String authToken, const char *serverUrl = SERVER_URL);

      /**
       * @brief Begins a stream describe request
       * 
       * The function makes and http request to the "/describe-stream" endpoint.
       * It captures a frame from the camera and sends as binary payload.
       * 
       * @return true if successful
       * @return false if unsuccessful
       */
      bool startRequest();

      /**
       * @brief loop to be called in the main loop
       * 
       * The loop handles receiving the response payload.
       * It does nothing if processing == false, or if there is no ongoing request.
       * 
       */
      void loop();
  };

  /**
   * @brief Client for websocket connections
   * 
   * This client maintains a websocket connections and handles piping data in and out of socket.
   * 
   */
  class LiveClient {
    private:
      Audio::AudioIO& audio;                  // Reference to the AudioIO object
      WebSocketsClient webSocket;             // Websocket client used for live connection
      String authToken;                       // Unique auth token for this device - EfuseMac
      const char *serverUrl;                  // Default SERVER_URL

      TaskHandle_t wsTaskHandle = nullptr;    // Handle of the websocket task spawn on successful connection

      /**
       * @brief Websocket task for sending and receiving data
       * 
       * The task, once spawned, stream the microphone input to the websocket and intermittenly sends images.
       * Once maximum duration of MAX_WEBSOCKET_TIME as been reached, the task will close the socket and kill itself.
       * 
       */
      static void websocketTaskWrapper(void *ptr);
      void websocketTask();

      /**
       * @brief Websocket event handler
       * 
       * Handles events received from websocket.
       * 
       * @param type type of event
       * @param payload data received from socket
       * @param length length of data received in bytes
       * @param ptr 
       */
      static void websocketEventWrapper(WStype_t type, uint8_t * payload, size_t length, void *ptr);
      void websocketEvent(WStype_t type, uint8_t * payload, size_t length);

    public:
      bool processing = false;              // True if a websocket process has been started

      /**
       * @brief Construct a new Live Client object
       * 
       * @param audioRef 
       * @param authToken 
       * @param serverUrl 
       */
      LiveClient(Audio::AudioIO& audioRef, String authToken, const char *serverUrl = SERVER_URL);

      /**
       * @brief Begins the websocket connection
       * 
       * This function begins the websocket connection with server and spawns a task for mananing data piping.
       * It does nothing if processing is already true.
       * 
       * @return true if successful
       * @return false if unsuccessful
       */
      bool beginLive();

      /**
       * @brief Ends the websocket connection
       * 
       * This function disconnects the ongoing the websocket connection and kills the task if it is running.
       * 
       */
      void endLive();

      /**
       * @brief Returns whether websocket is currently connected.
       * 
       * Return value of this method and processing is that this method returns true only if the socket is open and connected.
       * Unlike processing, it takes time for this method to return true after calling `begin_live`.
       * 
       * @return true if socket is connected
       * @return false if socket is not connected
       */
      bool isConnected();
  };

}