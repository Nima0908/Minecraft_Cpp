#include "auth_minecraft.hpp"
#include "http.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

MinecraftLoginResponse loginWithMinecraft(const std::string &userhash,
                                          const std::string &xstsToken) {
  std::string url =
      "https://api.minecraftservices.com/authentication/login_with_xbox";

  json body = {{"identityToken", "XBL3.0 x=" + userhash + ";" + xstsToken}};

  Headers headers = {"Content-Type: application/json",
                     "Accept: application/json"};

  std::string respStr = httpPost(url, body.dump(), headers);
  auto respJson = json::parse(respStr);

  MinecraftLoginResponse res;
  res.access_token = respJson["access_token"];

  return res;
}

bool checkMinecraftOwnership(const std::string &mcAccessToken) {
  std::string url = "https://api.minecraftservices.com/entitlements/mcstore";

  Headers headers = {"Authorization: Bearer " + mcAccessToken};

  std::string respStr = httpGet(url, headers);
  auto respJson = json::parse(respStr);

  if (respJson.contains("items") && respJson["items"].is_array()) {
    return !respJson["items"].empty();
  }

  return false;
}
