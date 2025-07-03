#pragma once
#include <string>

namespace mc::auth {

struct XstsResponse {
  std::string token;
  std::string userhash;
};

XstsResponse getXSTSToken(const std::string &xblToken);
} // namespace mc::auth
