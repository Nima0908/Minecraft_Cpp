#pragma once
#include <string>

namespace mc::auth {

struct MinecraftLoginResponse {
  std::string access_token;
};

MinecraftLoginResponse loginWithMinecraft(const std::string &userhash,
                                          const std::string &xstsToken);
bool checkMinecraftOwnership(const std::string &mcAccessToken);
} // namespace mc::auth
