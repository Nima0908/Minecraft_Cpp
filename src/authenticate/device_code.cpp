#include "device_code.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace mc::auth {

static size_t write_cb(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

DeviceCodeResponse requestDeviceCode(const std::string& clientId) {
    if (auto* curl = curl_easy_init()) {
        std::string response;
        std::string postFields = 
            "client_id=" + clientId +
            "&scope=XboxLive.signin%20offline_access";

        curl_easy_setopt(curl, CURLOPT_URL,
            "https://login.microsoftonline.com/common/oauth2/v2.0/devicecode");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        if (CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("CURL device_code request failed");
        }
        curl_easy_cleanup(curl);

        auto j = nlohmann::json::parse(response);
        if (j.contains("device_code") && j.contains("user_code") &&
            j.contains("verification_uri") && j.contains("interval"))
        {
            return {
                j["device_code"].get<std::string>(),
                j["user_code"].get<std::string>(),
                j["verification_uri"].get<std::string>(),
                j["interval"].get<int>()
            };
        } else {
            throw std::runtime_error("device_code response missing fields");
        }
    }
    throw std::runtime_error("CURL initialization failed");
}

} // namespace mc::auth
