#include "auth_device_code.hpp"
#include "http.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

DeviceCodeResponse requestDeviceCode(const std::string &clientId) {
  std::string url =
      "https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode";

  std::string postData =
      "client_id=" + clientId + "&scope=XboxLive.signin offline_access";

  Headers headers = {"Content-Type: application/x-www-form-urlencoded",
                     "Accept: application/json"};

  std::string respStr = httpPost(url, postData, headers);
  auto respJson = json::parse(respStr);

  DeviceCodeResponse res;
  res.device_code = respJson["device_code"];
  res.user_code = respJson["user_code"];
  res.verification_uri = respJson["verification_uri"];
  res.expires_in = respJson["expires_in"];
  res.interval = respJson["interval"];

  return res;
}

std::string pollToken(const std::string &clientId,
                      const std::string &deviceCode, int interval) {
  std::string url =
      "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";

  Headers headers = {"Content-Type: application/x-www-form-urlencoded",
                     "Accept: application/json"};

  while (true) {
    std::string postData =
        "grant_type=urn:ietf:params:oauth:grant-type:device_code"
        "&client_id=" +
        clientId + "&device_code=" + deviceCode;

    std::string respStr = httpPost(url, postData, headers);
    auto respJson = json::parse(respStr);

    if (respJson.contains("access_token")) {
      return respJson["access_token"];
    } else if (respJson.contains("error")) {
      std::string error = respJson["error"];
      if (error == "authorization_pending") {
        std::this_thread::sleep_for(std::chrono::seconds(interval));
      } else {
        throw std::runtime_error("Device code error: " + error);
      }
    } else {
      throw std::runtime_error("Unexpected device code response");
    }
  }
}
