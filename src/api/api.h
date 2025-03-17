#pragma once
#include <string.h>
#include "esp_camera.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

namespace API {

  const String SERVER_URL = "https://luna-backend-864401919642.us-east1.run.app/";

  class APIClient {
    private:
      String auth_token;
      String server_url;
      uint16_t timeout;

    public:
      APIClient(String auth_token, String server_url = SERVER_URL, uint16_t timeout = 10000);

      WiFiClient* request_description(WiFiClientSecure &client, uint8_t *buf, size_t len);
  };

}