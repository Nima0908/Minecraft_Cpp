#pragma once

#include <string>

namespace mc::auth {

struct TokenResponse {
    std::string access_token;
    std::string refresh_token;
};

TokenResponse pollForToken(const std::string& clientId, const std::string& deviceCode, int interval);

}

