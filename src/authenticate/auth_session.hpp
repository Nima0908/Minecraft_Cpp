// auth_session.hpp
#pragma once
#include <string>

namespace mc::auth {

void sendJoinServerRequest(
    const std::string &accessToken,
    const std::string &selectedProfile, // UUID without dashes
    const std::string &serverHash);

std::string getMinecraftUUID(const std::string &accessToken);
} // namespace mc::auth
