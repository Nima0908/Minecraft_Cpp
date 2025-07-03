#pragma once
#include <string>

namespace mc::auth {

struct XblResponse {
  std::string token;
  std::string userhash;
};

XblResponse authenticateWithXBL(const std::string &accessToken);
} // namespace mc::auth
