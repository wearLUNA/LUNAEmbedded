#include "api.h"

namespace API {

  const char *SERVER_URL = "luna-backend-864401919642.us-east1.run.app";

  APIClient::APIClient(String auth_token, const char *server_url, uint16_t timeout) {
    http = new HTTPClient();
    this->auth_token = auth_token;
    this->server_url = server_url;
    this->timeout = timeout;
  }

  const char *APIClient::get_host() {
    return this->server_url;
  }

  String APIClient::get_description_req_str(size_t payload_len) {
    String http_request =
      "POST " + String("/describe-stream") + " HTTP/1.1\r\n" // UNKNOWN ERROR CODE (0050) - crashing on HTTP/1.1 need to use HTTP/1.0
      + "Host: " + String(this->server_url) + "\r\n"
      + "Authorization: " + String(this->auth_token) + "\r\n"
      + "Accept-Encoding: gzip, deflate, br\r\n"
      + "Content-Type: image/jpg\r\n"
      + "Content-Length: " + payload_len + "\r\n"
      + "Connection: keep-alive\r\n" + "\r\n";

    return http_request;
  }
}