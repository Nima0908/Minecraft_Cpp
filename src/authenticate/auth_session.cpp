// auth_session.cpp
#include "auth_session.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

void sendJoinServerRequest(const std::string& accessToken,
                           const std::string& selectedProfile,
                           const std::string& serverHash) {
    nlohmann::json payload = {
        {"accessToken", accessToken},
        {"selectedProfile", selectedProfile},
        {"serverId", serverHash}
    };
    std::string data = payload.dump();

    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("CURL init failed");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, "https://sessionserver.mojang.com/session/minecraft/join");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Mojang join failed: " + std::string(curl_easy_strerror(res)));
    }
}

std::string getMinecraftUUID(const std::string& accessToken) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl init failed");

    std::string responseStr;
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.minecraftservices.com/minecraft/profile");
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* ptr, size_t size, size_t nmemb, std::string* data) {
            data->append(ptr, size * nmemb);
            return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) throw std::runtime_error("Failed to get profile");

    auto json = nlohmann::json::parse(responseStr);
    return json["id"];
}
