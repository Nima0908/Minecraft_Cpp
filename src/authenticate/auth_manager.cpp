#include "auth_manager.hpp"
#include "../network/http/http_handler.hpp"
#include "../util/logger.hpp"
#include <algorithm>
#include <boost/json.hpp>
#include <chrono>
#include <fstream>
#include <thread>

namespace json = boost::json;

namespace mc::auth {

AuthManager::AuthManager(const std::string &clientId,
                         const std::string &tokenFile,
                         mc::network::http::HttpHandler *httpHandler)
    : client_id(clientId), token_file(tokenFile), http_handler(httpHandler) {
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "AuthManager initialized with client ID: ", clientId);
}

bool AuthManager::authenticate() {
  if (loadTokens()) {
    mc::utils::log(mc::utils::LogLevel::INFO, "Found existing tokens");
    if (isTokenExpired()) {
      mc::utils::log(mc::utils::LogLevel::INFO,
                     "Tokens expired, attempting refresh...");
      if (refreshTokens()) {
        mc::utils::log(mc::utils::LogLevel::INFO,
                       "Tokens refreshed successfully");
        saveTokens();
        return true;
      } else {
        mc::utils::log(mc::utils::LogLevel::WARN,
                       "Token refresh failed, starting new authentication");
        clearTokens();
      }
    } else {
      mc::utils::log(mc::utils::LogLevel::INFO, "Using cached tokens");
      return true;
    }
  }

  try {
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Starting Microsoft authentication flow...");

    auto deviceData = requestDeviceCode();
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Please visit: ", deviceData.verification_uri);
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Enter code: ", deviceData.user_code);

    tokens = pollForToken(deviceData);

    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Authentication successful for user: ", tokens->username);
    saveTokens();
    return true;

  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Authentication failed: ", e.what());
    return false;
  }
}

DeviceCodeData AuthManager::requestDeviceCode() {
  std::string url =
      "https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode";
  std::string body =
      "client_id=" + client_id + "&scope=XboxLive.signin offline_access";

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/x-www-form-urlencoded"},
      {"Accept", "application/json"}};

  try {
    std::string response = http_handler->post(url, body, headers);
    json::object json_resp = json::parse(response).as_object();

    return DeviceCodeData{
        .device_code = std::string(json_resp["device_code"].as_string()),
        .user_code = std::string(json_resp["user_code"].as_string()),
        .verification_uri =
            std::string(json_resp["verification_uri"].as_string()),
        .expires_in = static_cast<int>(json_resp["expires_in"].as_int64()),
        .interval = static_cast<int>(json_resp["interval"].as_int64())};

  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to request device code: ", e.what());
    throw;
  }
}

AuthTokens AuthManager::pollForToken(const DeviceCodeData &deviceData) {
  std::string url =
      "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";
  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/x-www-form-urlencoded"},
      {"Accept", "application/json"}};

  std::string msToken, refreshToken;
  int expiresIn = 0;

  while (true) {
    std::string body = "grant_type=urn:ietf:params:oauth:grant-type:device_code"
                       "&client_id=" +
                       client_id + "&device_code=" + deviceData.device_code;

    try {
      std::string response = http_handler->post(url, body, headers);
      json::object json_resp = json::parse(response).as_object();

      if (json_resp.contains("access_token")) {
        msToken = std::string(json_resp["access_token"].as_string());
        refreshToken = json_resp.if_contains("refresh_token")
                           ? std::string(json_resp["refresh_token"].as_string())
                           : "";
        expiresIn = json_resp.if_contains("expires_in")
                        ? static_cast<int>(json_resp["expires_in"].as_int64())
                        : 3600;
        break;
      } else if (json_resp.contains("error")) {
        std::string error = std::string(json_resp["error"].as_string());
        if (error == "authorization_pending") {
          std::this_thread::sleep_for(
              std::chrono::seconds(deviceData.interval));
        } else {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Device code error: ", error);
          throw std::runtime_error("Device code error: " + error);
        }
      } else {
        throw std::runtime_error("Unexpected response during token polling");
      }
    } catch (const std::exception &e) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Error during token polling: ", e.what());
      throw;
    }
  }

  std::string xblResponse = authenticateWithXbox(msToken);
  json::object xblJson = json::parse(xblResponse).as_object();
  std::string xblToken = std::string(xblJson["Token"].as_string());
  std::string userHash = getUserHash(xblResponse);
  std::string xstsToken = getXstsToken(xblToken);
  std::string mcToken = getMinecraftToken(xstsToken, userHash);

  if (!checkGameOwnership(mcToken)) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Minecraft not owned on this account");
    throw std::runtime_error("Minecraft not owned");
  }

  auto [uuid, username] = getPlayerProfile(mcToken);

  auto expiresAt =
      std::chrono::system_clock::now() + std::chrono::seconds(expiresIn - 300);

  return AuthTokens{.access_token = msToken,
                    .refresh_token = refreshToken,
                    .minecraft_token = mcToken,
                    .uuid = uuid,
                    .username = username,
                    .expires_at = expiresAt,
                    .expires_in = expiresIn};
}

bool AuthManager::refreshTokens() {
  if (!tokens || tokens->refresh_token.empty()) {
    mc::utils::log(mc::utils::LogLevel::ERROR, "No refresh token available");
    return false;
  }

  try {
    std::string url =
        "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";
    std::string body = "grant_type=refresh_token"
                       "&client_id=" +
                       client_id + "&refresh_token=" + tokens->refresh_token +
                       "&scope=XboxLive.signin offline_access";

    mc::network::http::HttpHandler::Headers headers = {
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "application/json"}};

    std::string response = http_handler->post(url, body, headers);
    json::object json_resp = json::parse(response).as_object();

    if (!json_resp.contains("access_token")) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Refresh token response missing access_token");
      return false;
    }

    std::string newMsToken = std::string(json_resp["access_token"].as_string());
    std::string newRefreshToken =
        json_resp.if_contains("refresh_token")
            ? std::string(json_resp["refresh_token"].as_string())
            : tokens->refresh_token;
    int expiresIn = json_resp.if_contains("expires_in")
                        ? static_cast<int>(json_resp["expires_in"].as_int64())
                        : 3600;

    std::string xblResponse = authenticateWithXbox(newMsToken);
    json::object xblJson = json::parse(xblResponse).as_object();
    std::string xblToken = std::string(xblJson["Token"].as_string());
    std::string userHash = getUserHash(xblResponse);
    std::string xstsToken = getXstsToken(xblToken);
    std::string newMcToken = getMinecraftToken(xstsToken, userHash);

    tokens->access_token = newMsToken;
    tokens->refresh_token = newRefreshToken;
    tokens->minecraft_token = newMcToken;
    tokens->expires_at = std::chrono::system_clock::now() +
                         std::chrono::seconds(expiresIn - 300);
    tokens->expires_in = expiresIn;

    return true;

  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Token refresh failed: ", e.what());
    return false;
  }
}

std::string AuthManager::authenticateWithXbox(const std::string &msToken) {
  std::string url = "https://user.auth.xboxlive.com/user/authenticate";

  json::object body;
  body["Properties"] = {{"AuthMethod", "RPS"},
                        {"SiteName", "user.auth.xboxlive.com"},
                        {"RpsTicket", "d=" + msToken}};
  body["RelyingParty"] = "http://auth.xboxlive.com";
  body["TokenType"] = "JWT";

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}, {"Accept", "application/json"}};

  return http_handler->post(url, json::serialize(body), headers);
}

std::string AuthManager::getUserHash(const std::string &xblResponse) {
  try {
    auto obj = json::parse(xblResponse).as_object();
    return std::string(obj["DisplayClaims"]
                           .as_object()["xui"]
                           .as_array()[0]
                           .as_object()["uhs"]
                           .as_string());
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to parse user hash: ", e.what());
    throw;
  }
}

std::string AuthManager::getXstsToken(const std::string &xblToken) {
  std::string url = "https://xsts.auth.xboxlive.com/xsts/authorize";

  json::object body;
  body["Properties"] = {{"SandboxId", "RETAIL"}, {"UserTokens", {xblToken}}};
  body["RelyingParty"] = "rp://api.minecraftservices.com/";
  body["TokenType"] = "JWT";

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}, {"Accept", "application/json"}};

  std::string response =
      http_handler->post(url, json::serialize(body), headers);
  auto obj = json::parse(response).as_object();

  return std::string(obj["Token"].as_string());
}

std::string AuthManager::getMinecraftToken(const std::string &xstsToken,
                                           const std::string &userHash) {
  std::string url =
      "https://api.minecraftservices.com/authentication/login_with_xbox";

  json::object body;
  body["identityToken"] = "XBL3.0 x=" + userHash + ";" + xstsToken;

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}, {"Accept", "application/json"}};

  std::string response =
      http_handler->post(url, json::serialize(body), headers);
  auto obj = json::parse(response).as_object();
  return std::string(obj["access_token"].as_string());
}

bool AuthManager::checkGameOwnership(const std::string &mcToken) {
  std::string url = "https://api.minecraftservices.com/entitlements/mcstore";

  mc::network::http::HttpHandler::Headers headers = {
      {"Authorization", "Bearer " + mcToken}};

  std::string response = http_handler->get(url, headers);
  auto obj = json::parse(response).as_object();
  return obj.contains("items") && obj["items"].is_array() &&
         !obj["items"].as_array().empty();
}

std::pair<std::string, std::string>
AuthManager::getPlayerProfile(const std::string &mcToken) {
  std::string url = "https://api.minecraftservices.com/minecraft/profile";

  mc::network::http::HttpHandler::Headers headers = {
      {"Authorization", "Bearer " + mcToken}};

  std::string response = http_handler->get(url, headers);
  auto obj = json::parse(response).as_object();

  return {std::string(obj["id"].as_string()),
          std::string(obj["name"].as_string())};
}

void AuthManager::joinServer(const std::string &serverHash) {
  if (!isAuthenticated()) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Cannot join server: not authenticated");
    return;
  }

  if (isTokenExpired()) {
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Tokens expired, refreshing before joining server...");
    if (!refreshTokens()) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Failed to refresh tokens before joining server");
      return;
    }
    saveTokens();
  }

  std::string url = "https://sessionserver.mojang.com/session/minecraft/join";

  std::string cleanUuid = tokens->uuid;
  cleanUuid.erase(std::remove(cleanUuid.begin(), cleanUuid.end(), '-'),
                  cleanUuid.end());

  json::object body;
  body["accessToken"] = tokens->minecraft_token;
  body["selectedProfile"] = cleanUuid;
  body["serverId"] = serverHash;

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}};

  http_handler->post(url, json::serialize(body), headers);
}

bool AuthManager::isTokenExpired() const {
  if (!tokens)
    return true;
  return std::chrono::system_clock::now() >= tokens->expires_at;
}

void AuthManager::saveTokens() const {
  if (!tokens)
    return;

  json::object tokenData;
  tokenData["access_token"] = tokens->access_token;
  tokenData["refresh_token"] = tokens->refresh_token;
  tokenData["minecraft_token"] = tokens->minecraft_token;
  tokenData["uuid"] = tokens->uuid;
  tokenData["username"] = tokens->username;
  tokenData["expires_in"] = tokens->expires_in;
  tokenData["expires_at"] =
      static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                               tokens->expires_at.time_since_epoch())
                               .count());

  std::ofstream file(token_file);
  if (file.is_open())
    file << json::serialize(tokenData);
}

bool AuthManager::loadTokens() {
  std::ifstream file(token_file);
  if (!file.is_open())
    return false;

  try {
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    json::object tokenData = json::parse(content).as_object();

    auto expiresAt = std::chrono::system_clock::time_point(
        std::chrono::seconds(tokenData["expires_at"].as_int64()));

    tokens = AuthTokens{
        .access_token = std::string(tokenData["access_token"].as_string()),
        .refresh_token = std::string(tokenData["refresh_token"].as_string()),
        .minecraft_token =
            std::string(tokenData["minecraft_token"].as_string()),
        .uuid = std::string(tokenData["uuid"].as_string()),
        .username = std::string(tokenData["username"].as_string()),
        .expires_at = expiresAt,
        .expires_in = static_cast<int>(tokenData["expires_in"].as_int64())};

    return true;
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to load tokens: ", e.what());
    return false;
  }
}

void AuthManager::clearTokens() {
  tokens.reset();
  std::remove(token_file.c_str());
  mc::utils::log(mc::utils::LogLevel::INFO, "Tokens cleared and file deleted");
}

} // namespace mc::auth
