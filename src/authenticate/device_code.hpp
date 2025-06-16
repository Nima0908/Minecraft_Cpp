#pragma once

#include <string>

namespace mc::auth {

struct DeviceCodeResponse {
    std::string device_code;
    std::string user_code;
    std::string verification_uri;
    int interval;
};

DeviceCodeResponse requestDeviceCode(const std::string& clientId);

}
