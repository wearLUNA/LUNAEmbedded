#include "api.h"

namespace API {

  APIClient::APIClient(String auth_token, String server_url = SERVER_URL, uint16_t timeout = 10000) {
    this->auth_token = auth_token;
    this->server_url = server_url;
    this->timeout = timeout;
  }

  WiFiClient* APIClient::request_description(WiFiClientSecure &client, uint8_t *buf, size_t len) {
    HTTPClient http;

    http.begin(client, this->server_url + "describe-stream");
    http.setTimeout(this->timeout);

    http.addHeader("Content-Type", "image/jpg");
    http.addHeader("Authorization", this->auth_token);

    const char *header_keys[] = {"Content-Length"};
    const size_t hkeys_count = sizeof(header_keys) / sizeof(header_keys[0]);
    http.collectHeaders(header_keys, hkeys_count);

    int httpResponseCode = http.POST(buf, len);
    if (httpResponseCode != 200) {
      return NULL;
    }


    return http.getStreamPtr();
  }
}