#pragma once
#include <string>
#include <vector>

struct XstsResponse {
    std::string token;
    std::string userhash;
};

XstsResponse getXSTSToken(const std::string& xblToken);
