#include "auth_xsts.hpp"
#include "http.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace mc::auth {

XstsResponse getXSTSToken(const std::string &xblToken) {
  std::string url = "https://xsts.auth.xboxlive.com/xsts/authorize";

  json body = {
      {"Properties", {{"SandboxId", "RETAIL"}, {"UserTokens", {xblToken}}}},
      {"RelyingParty", "rp://api.minecraftservices.com/"},
      {"TokenType", "JWT"}};

  Headers headers = {"Content-Type: application/json",
                     "Accept: application/json"};

  std::string respStr = httpPost(url, body.dump(), headers);
  auto respJson = json::parse(respStr);

  XstsResponse res;
  res.token = respJson["Token"];
  res.userhash = respJson["DisplayClaims"]["xui"][0]["uhs"];

  return res;
}
} // namespace mc::auth
