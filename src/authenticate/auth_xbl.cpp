#include "auth_xbl.hpp"
#include "http.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

XblResponse authenticateWithXBL(const std::string &accessToken) {
  std::string url = "https://user.auth.xboxlive.com/user/authenticate";

  json body = {{"Properties",
                {{"AuthMethod", "RPS"},
                 {"SiteName", "user.auth.xboxlive.com"},
                 {"RpsTicket", "d=" + accessToken}}},
               {"RelyingParty", "http://auth.xboxlive.com"},
               {"TokenType", "JWT"}};

  Headers headers = {"Content-Type: application/json",
                     "Accept: application/json"};

  std::string respStr = httpPost(url, body.dump(), headers);
  auto respJson = json::parse(respStr);

  XblResponse res;
  res.token = respJson["Token"];
  res.userhash = respJson["DisplayClaims"]["xui"][0]["uhs"];

  return res;
}
