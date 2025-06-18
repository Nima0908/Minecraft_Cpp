// auth_session.hpp
#pragma once
#include <string>

void sendJoinServerRequest(const std::string& accessToken,
                           const std::string& selectedProfile,  // UUID without dashes
                           const std::string& serverHash);

std::string getMinecraftUUID(const std::string& accessToken);
