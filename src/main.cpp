#include "authenticate/auth_manager.hpp"
#include "util/log_level.hpp"
#include "util/logger.hpp"

#include <stdint.h>


int main (int argc, char *argv[]) {
  constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";
  mc::auth::AuthManager auth(CLIENT_ID, "minecraft_tokens.json");
    
    // Authenticate - this will:
    // 1. Check for existing token file
    // 2. If found, check if tokens are expired
    // 3. If expired, try to refresh using refresh token
    // 4. If refresh fails or no tokens exist, start new device code flow
    if (!auth.authenticate()) {
        mc::utils::log(mc::utils::LogLevel::ERROR, "Authentication failed");
        return 1;
    } 
  return 0;
}
