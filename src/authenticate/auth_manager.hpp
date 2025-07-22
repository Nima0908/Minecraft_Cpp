// auth_manager.hpp
#pragma once
#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>
#include "http_client.hpp"

namespace mc::auth {

struct AuthTokens {
    std::string access_token;
    std::string refresh_token;
    std::string minecraft_token;
    std::string uuid;
    std::string username;
    std::chrono::system_clock::time_point expires_at;
    int expires_in = 0;
};

struct DeviceCodeData {
    std::string device_code;
    std::string user_code;
    std::string verification_uri;
    int expires_in;
    int interval;
};

class AuthManager {
private:
    HttpClient http;
    std::string client_id;
    std::optional<AuthTokens> tokens;
    std::string token_file;
    
public:
    explicit AuthManager(const std::string& clientId, const std::string& tokenFile = "minecraft_auth.json");
    
    // Main authentication flow
    bool authenticate();
    bool isAuthenticated() const { return tokens.has_value(); }
    const AuthTokens& getTokens() const { return tokens.value(); }
    
    // Server authentication
    void joinServer(const std::string& serverHash);
    
    // Token management
    void saveTokens() const;
    bool loadTokens();
    void clearTokens();
    bool refreshTokens();
    
private:
    DeviceCodeData requestDeviceCode();
    AuthTokens pollForToken(const DeviceCodeData& deviceData);
    std::string authenticateWithXbox(const std::string& msToken);
    std::string getXstsToken(const std::string& xblToken);
    std::string getMinecraftToken(const std::string& xstsToken, const std::string& userHash);
    std::string getUserHash(const std::string& xblResponse);
    bool checkGameOwnership(const std::string& mcToken);
    std::pair<std::string, std::string> getPlayerProfile(const std::string& mcToken);
    bool isTokenExpired() const;
};

} // namespace mc::auth
