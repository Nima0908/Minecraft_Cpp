#include "token_poll.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace mc::auth {

static size_t write_cb(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

TokenResponse pollForToken(const std::string& clientId, const std::string& deviceCode, int interval) {
    if (auto* curl = curl_easy_init()) {
        std::string response;
        while (true) {
            response.clear();
            std::string postFields =
                "grant_type=urn:ietf:params:oauth:grant-type:device_code"
                "&client_id=" + clientId +
                "&device_code=" + deviceCode;

            curl_easy_setopt(curl, CURLOPT_URL,
                "https://login.microsoftonline.com/common/oauth2/v2.0/token");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            if (CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
                curl_easy_cleanup(curl);
                throw std::runtime_error("CURL token_poll request failed");
            }

            auto j = nlohmann::json::parse(response);
            if (j.contains("access_token")) {
                curl_easy_cleanup(curl);
                return {
                    j["access_token"].get<std::string>(),
                    j.value("refresh_token", "")
                };
            }

            std::string err = j.value("error", "");
            if (err == "authorization_pending") {
                std::this_thread::sleep_for(std::chrono::seconds(interval));
                continue;
            } else if (err == "slow_down") {
                interval += 5;
                std::this_thread::sleep_for(std::chrono::seconds(interval));
                continue;
            } else {
                curl_easy_cleanup(curl);
                throw std::runtime_error("Token poll error: " + err);
            }
        }
    }
    throw std::runtime_error("CURL initialization failed");
}

} // namespace mc::auth
