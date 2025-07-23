// auth_manager.cpp
#include "auth_manager.hpp"
#include "../network/http/http_handler.hpp"
#include "../util/logger.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>

using json = nlohmann::json;

namespace mc::auth {

AuthManager::AuthManager(const std::string &clientId,
                         const std::string &tokenFile,
                         mc::network::http::HttpHandler *httpHandler)
    : client_id(clientId), token_file(tokenFile), http_handler(httpHandler) {
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "AuthManager initialized with client ID: ", clientId);
}

bool AuthManager::authenticate() {
  // First, try to load existing tokens
  if (loadTokens()) {
    mc::utils::log(mc::utils::LogLevel::INFO, "Found existing tokens");

    // Check if tokens need refreshing
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

  // If no cached tokens or refresh failed, start fresh authentication
  try {
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Starting Microsoft authentication flow...");

    // Step 1: Get device code
    auto deviceData = requestDeviceCode();
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Please visit: ", deviceData.verification_uri);
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Enter code: ", deviceData.user_code);

    // Step 2: Poll for access token and get all tokens
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

  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Requesting device code from: ", url);

  try {
    std::string response = http_handler->post(url, body, headers);
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Device code response body: ", response);

    auto json_resp = json::parse(response);

    return DeviceCodeData{.device_code = json_resp["device_code"],
                          .user_code = json_resp["user_code"],
                          .verification_uri = json_resp["verification_uri"],
                          .expires_in = json_resp["expires_in"],
                          .interval = json_resp["interval"]};
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
      auto json_resp = json::parse(response);

      if (json_resp.contains("access_token")) {
        msToken = json_resp["access_token"];
        refreshToken = json_resp.contains("refresh_token")
                           ? json_resp["refresh_token"]
                           : "";
        expiresIn = json_resp.contains("expires_in")
                        ? json_resp["expires_in"].get<int>()
                        : 3600;
        mc::utils::log(mc::utils::LogLevel::INFO, "Microsoft token obtained");
        break;
      } else if (json_resp.contains("error")) {
        std::string error = json_resp["error"];
        if (error == "authorization_pending") {
          mc::utils::log(mc::utils::LogLevel::DEBUG,
                         "Waiting for user authorization...");
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
    } catch (const json::exception &e) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "JSON parsing error during token polling: ", e.what());
      throw std::runtime_error("Invalid JSON response during token polling");
    }
  }

  // Now continue with the Xbox Live and Minecraft authentication chain
  std::string xblResponse = authenticateWithXbox(msToken);
  auto xblJson = json::parse(xblResponse);
  std::string xblToken = xblJson["Token"];
  std::string userHash = getUserHash(xblResponse);
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Xbox Live authentication complete");

  std::string xstsToken = getXstsToken(xblToken);
  mc::utils::log(mc::utils::LogLevel::DEBUG, "XSTS token obtained");

  std::string mcToken = getMinecraftToken(xstsToken, userHash);
  mc::utils::log(mc::utils::LogLevel::DEBUG, "Minecraft token obtained");

  if (!checkGameOwnership(mcToken)) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Minecraft not owned on this account");
    throw std::runtime_error("Minecraft not owned on this account");
  }

  auto [uuid, username] = getPlayerProfile(mcToken);
  mc::utils::log(mc::utils::LogLevel::INFO, "Player profile: ", username, " (",
                 uuid, ")");

  auto expiresAt = std::chrono::system_clock::now() +
                   std::chrono::seconds(expiresIn - 300); // 5 min buffer

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
    auto json_resp = json::parse(response);

    if (!json_resp.contains("access_token")) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Refresh token response missing access_token");
      return false;
    }

    std::string newMsToken = json_resp["access_token"];
    std::string newRefreshToken =
        json_resp.contains("refresh_token")
            ? json_resp["refresh_token"].get<std::string>()
            : tokens->refresh_token;
    int expiresIn = json_resp.contains("expires_in")
                        ? json_resp["expires_in"].get<int>()
                        : 3600;

    mc::utils::log(mc::utils::LogLevel::INFO, "Microsoft token refreshed");

    // Refresh the entire authentication chain
    std::string xblResponse = authenticateWithXbox(newMsToken);
    auto xblJson = json::parse(xblResponse);
    std::string xblToken = xblJson["Token"];
    std::string userHash = getUserHash(xblResponse);

    std::string xstsToken = getXstsToken(xblToken);
    std::string newMcToken = getMinecraftToken(xstsToken, userHash);

    // Update tokens
    tokens->access_token = newMsToken;
    tokens->refresh_token = newRefreshToken;
    tokens->minecraft_token = newMcToken;
    tokens->expires_at = std::chrono::system_clock::now() +
                         std::chrono::seconds(expiresIn - 300);
    tokens->expires_in = expiresIn;

    mc::utils::log(mc::utils::LogLevel::INFO,
                   "All tokens refreshed successfully");
    return true;

  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Token refresh failed: ", e.what());
    return false;
  }
}

std::string AuthManager::authenticateWithXbox(const std::string &msToken) {
  std::string url = "https://user.auth.xboxlive.com/user/authenticate";

  json body = {{"Properties",
                {{"AuthMethod", "RPS"},
                 {"SiteName", "user.auth.xboxlive.com"},
                 {"RpsTicket", "d=" + msToken}}},
               {"RelyingParty", "http://auth.xboxlive.com"},
               {"TokenType", "JWT"}};

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}, {"Accept", "application/json"}};

  try {
    return http_handler->post(url, body.dump(), headers);
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Xbox authentication failed: ", e.what());
    throw;
  }
}

std::string AuthManager::getUserHash(const std::string &xblResponse) {
  try {
    auto json_resp = json::parse(xblResponse);
    return json_resp["DisplayClaims"]["xui"][0]["uhs"];
  } catch (const json::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to parse user hash from XBL response: ", e.what());
    throw std::runtime_error("Invalid XBL response format");
  }
}

std::string AuthManager::getXstsToken(const std::string &xblToken) {
  std::string url = "https://xsts.auth.xboxlive.com/xsts/authorize";

  json body = {
      {"Properties", {{"SandboxId", "RETAIL"}, {"UserTokens", {xblToken}}}},
      {"RelyingParty", "rp://api.minecraftservices.com/"},
      {"TokenType", "JWT"}};

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}, {"Accept", "application/json"}};

  try {
    std::string response = http_handler->post(url, body.dump(), headers);
    auto json_resp = json::parse(response);

    if (!json_resp.contains("Token")) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "XSTS response missing Token field");
      throw std::runtime_error("Invalid XSTS response");
    }

    return json_resp["Token"];
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "XSTS authentication failed: ", e.what());
    throw;
  }
}

std::string AuthManager::getMinecraftToken(const std::string &xstsToken,
                                           const std::string &userHash) {
  std::string url =
      "https://api.minecraftservices.com/authentication/login_with_xbox";

  json body = {{"identityToken", "XBL3.0 x=" + userHash + ";" + xstsToken}};

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}, {"Accept", "application/json"}};

  try {
    std::string response = http_handler->post(url, body.dump(), headers);
    auto json_resp = json::parse(response);

    if (!json_resp.contains("access_token")) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Minecraft authentication response missing access_token");
      throw std::runtime_error("Invalid Minecraft authentication response");
    }

    return json_resp["access_token"];
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Minecraft authentication failed: ", e.what());
    throw;
  }
}

bool AuthManager::checkGameOwnership(const std::string &mcToken) {
  std::string url = "https://api.minecraftservices.com/entitlements/mcstore";

  mc::network::http::HttpHandler::Headers headers = {
      {"Authorization", "Bearer " + mcToken}};

  try {
    std::string response = http_handler->get(url, headers);
    auto json_resp = json::parse(response);

    return json_resp.contains("items") && json_resp["items"].is_array() &&
           !json_resp["items"].empty();
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to check game ownership: ", e.what());
    return false;
  }
}

std::pair<std::string, std::string>
AuthManager::getPlayerProfile(const std::string &mcToken) {
  std::string url = "https://api.minecraftservices.com/minecraft/profile";

  mc::network::http::HttpHandler::Headers headers = {
      {"Authorization", "Bearer " + mcToken}};

  try {
    std::string response = http_handler->get(url, headers);
    auto json_resp = json::parse(response);

    if (!json_resp.contains("id") || !json_resp.contains("name")) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Profile information not found in response");
      throw std::runtime_error("Profile information not found in response");
    }

    return {json_resp["id"], json_resp["name"]};
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to get player profile: ", e.what());
    throw;
  }
}

void AuthManager::joinServer(const std::string &serverHash) {
  if (!isAuthenticated()) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Cannot join server: not authenticated");
    throw std::runtime_error("Not authenticated");
  }

  // Check if tokens need refreshing before joining
  if (isTokenExpired()) {
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Tokens expired, refreshing before joining server...");
    if (!refreshTokens()) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Failed to refresh tokens before joining server");
      throw std::runtime_error("Failed to refresh expired tokens");
    }
    saveTokens();
  }

  std::string url = "https://sessionserver.mojang.com/session/minecraft/join";

  // Remove dashes from UUID
  std::string cleanUuid = tokens->uuid;
  cleanUuid.erase(std::remove(cleanUuid.begin(), cleanUuid.end(), '-'),
                  cleanUuid.end());

  json body = {{"accessToken", tokens->minecraft_token},
               {"selectedProfile", cleanUuid},
               {"serverId", serverHash}};

  mc::network::http::HttpHandler::Headers headers = {
      {"Content-Type", "application/json"}};

  try {
    http_handler->post(url, body.dump(), headers);
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Successfully joined server with hash: ", serverHash);
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to join server: ", e.what());
    throw;
  }
}

bool AuthManager::isTokenExpired() const {
  if (!tokens)
    return true;

  auto now = std::chrono::system_clock::now();
  bool expired = now >= tokens->expires_at;

  if (expired) {
    mc::utils::log(mc::utils::LogLevel::DEBUG, "Tokens are expired");
  } else {
    auto remaining = std::chrono::duration_cast<std::chrono::minutes>(
        tokens->expires_at - now);
    mc::utils::log(mc::utils::LogLevel::DEBUG, "Tokens valid for ",
                   remaining.count(), " more minutes");
  }

  return expired;
}

void AuthManager::saveTokens() const {
  if (!tokens) {
    mc::utils::log(mc::utils::LogLevel::WARN, "No tokens to save");
    return;
  }

  json tokenData = {
      {"access_token", tokens->access_token},
      {"refresh_token", tokens->refresh_token},
      {"minecraft_token", tokens->minecraft_token},
      {"uuid", tokens->uuid},
      {"username", tokens->username},
      {"expires_in", tokens->expires_in},
      {"expires_at", std::chrono::duration_cast<std::chrono::seconds>(
                         tokens->expires_at.time_since_epoch())
                         .count()}};

  std::ofstream file(token_file);
  if (!file.is_open()) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Cannot open file for saving tokens: ", token_file);
    return;
  }

  file << tokenData.dump(4);
  mc::utils::log(mc::utils::LogLevel::DEBUG, "Tokens saved to: ", token_file);
}

bool AuthManager::loadTokens() {
  std::ifstream file(token_file);
  if (!file.is_open()) {
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "No token file found: ", token_file);
    return false;
  }

  try {
    json tokenData;
    file >> tokenData;

    auto expiresAt =
        std::chrono::system_clock::from_time_t(tokenData["expires_at"]);

    tokens = AuthTokens{.access_token = tokenData["access_token"],
                        .refresh_token = tokenData["refresh_token"],
                        .minecraft_token = tokenData["minecraft_token"],
                        .uuid = tokenData["uuid"],
                        .username = tokenData["username"],
                        .expires_at = expiresAt,
                        .expires_in = tokenData["expires_in"]};

    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Tokens loaded for user: ", tokens->username);
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
