#pragma once
#include <string>

struct MinecraftLoginResponse {
    std::string access_token;
};

MinecraftLoginResponse loginWithMinecraft(const std::string& userhash, const std::string& xstsToken);
bool checkMinecraftOwnership(const std::string& mcAccessToken);
