#pragma once
#include <string>

struct DeviceCodeResponse {
    std::string device_code;
    std::string user_code;
    std::string verification_uri;
    int expires_in;
    int interval;
};

DeviceCodeResponse requestDeviceCode(const std::string& clientId);
std::string pollToken(const std::string& clientId, const std::string& deviceCode, int interval);
