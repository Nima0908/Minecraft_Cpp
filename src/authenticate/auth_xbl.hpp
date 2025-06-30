#pragma once
#include <string>

struct XblResponse {
  std::string token;
  std::string userhash;
};

XblResponse authenticateWithXBL(const std::string &accessToken);
