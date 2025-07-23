// auth_manager.hpp
#pragma once

#include "../network/http/http_handler.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>

namespace net = boost::asio;

namespace mc::auth {

struct DeviceCodeData {
  std::string device_code;
  std::string user_code;
  std::string verification_uri;
  int expires_in;
  int interval;
};

struct AuthTokens {
  std::string access_token;
  std::string refresh_token;
  std::string minecraft_token;
  std::string uuid;
  std::string username;
  std::chrono::system_clock::time_point expires_at;
  int expires_in;
};

class AuthManager {
public:
  AuthManager(const std::string &clientId, const std::string &tokenFile,
              mc::network::http::HttpHandler *httpHandler);
  bool authenticate();
  void joinServer(const std::string &serverHash);

  bool isAuthenticated() const {
    return tokens.has_value() && !isTokenExpired();
  }

  std::string getUsername() const {
    return tokens ? tokens->username : std::string{};
  }

  std::string getUuid() const { return tokens ? tokens->uuid : std::string{}; }

  void clearTokens();

private:
  DeviceCodeData requestDeviceCode();

  AuthTokens pollForToken(const DeviceCodeData &deviceData);

  bool refreshTokens();

  std::string authenticateWithXbox(const std::string &msToken);

  std::string getUserHash(const std::string &xblResponse);

  std::string getXstsToken(const std::string &xblToken);

  std::string getMinecraftToken(const std::string &xstsToken,
                                const std::string &userHash);

  bool checkGameOwnership(const std::string &mcToken);

  std::pair<std::string, std::string>
  getPlayerProfile(const std::string &mcToken);

  bool isTokenExpired() const;

  void saveTokens() const;

  bool loadTokens();

  std::string client_id;
  std::string token_file;
  mc::network::http::HttpHandler *http_handler; // now a pointer

  std::optional<AuthTokens> tokens;
};

} // namespace mc::auth
