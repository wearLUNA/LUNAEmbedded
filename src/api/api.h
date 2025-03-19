#pragma once
#include <string.h>
#include "esp_camera.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "audio/audio.h"

namespace API {

  extern const char *SERVER_URL;

  class APIClient {
    private:
      HTTPClient *http;
      String auth_token;
      const char *server_url;
      uint16_t timeout;

    public:
      APIClient(String auth_token, const char *server_url = SERVER_URL, uint16_t timeout = 10000);

      const char *get_host();

      String get_description_req_str(size_t payload_len);
  };

}