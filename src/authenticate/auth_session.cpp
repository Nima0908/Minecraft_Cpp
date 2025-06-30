// auth_session.cpp
#include "auth_session.hpp"
#include "../util/logger.hpp" // Assuming you have a logging utility with mc::log
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

void sendJoinServerRequest(const std::string &accessToken,
                           const std::string &selectedProfile,
                           const std::string &serverHash) {
  nlohmann::json payload = {{"accessToken", accessToken},
                            {"selectedProfile", selectedProfile},
                            {"serverId", serverHash}};
  std::string data = payload.dump();

  mc::log(mc::LogLevel::DEBUG, "Sending Join Server Request");
  mc::log(mc::LogLevel::DEBUG, "Payload: " + data);

  CURL *curl = curl_easy_init();
  if (!curl) {
    mc::log(mc::LogLevel::ERROR, "CURL init failed");
    throw std::runtime_error("CURL init failed");
  }

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://sessionserver.mojang.com/session/minecraft/join");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    mc::log(mc::LogLevel::ERROR,
            "Mojang join failed: " + std::string(curl_easy_strerror(res)));
  } else {
    mc::log(mc::LogLevel::DEBUG, "Join server request sent successfully");
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    throw std::runtime_error("Mojang join failed: " +
                             std::string(curl_easy_strerror(res)));
  }
}

std::string getMinecraftUUID(const std::string &accessToken) {
  mc::log(mc::LogLevel::DEBUG, "Fetching Minecraft UUID from access token");

  CURL *curl = curl_easy_init();
  if (!curl) {
    mc::log(mc::LogLevel::ERROR, "curl init failed");
    throw std::runtime_error("curl init failed");
  }

  std::string responseStr;
  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://api.minecraftservices.com/minecraft/profile");
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  struct curl_slist *headers = nullptr;
  std::string authHeader = "Authorization: Bearer " + accessToken;
  mc::log(mc::LogLevel::DEBUG, "Using header: " + authHeader);
  headers = curl_slist_append(headers, authHeader.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(
      curl, CURLOPT_WRITEFUNCTION,
      +[](char *ptr, size_t size, size_t nmemb, std::string *data) {
        data->append(ptr, size * nmemb);
        return size * nmemb;
      });
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    mc::log(mc::LogLevel::ERROR,
            "Failed to get profile: " + std::string(curl_easy_strerror(res)));
  } else {
    mc::log(mc::LogLevel::DEBUG, "Profile response: " + responseStr);
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    throw std::runtime_error("Failed to get profile");
  }

  try {
    auto json = nlohmann::json::parse(responseStr);
    if (json.contains("id")) {
      mc::log(mc::LogLevel::DEBUG,
              "UUID parsed from profile: " + json["id"].get<std::string>());
      return json["id"];
    } else {
      mc::log(mc::LogLevel::ERROR, "UUID not found in profile response");
      throw std::runtime_error("UUID not found in profile response");
    }
  } catch (const std::exception &e) {
    mc::log(mc::LogLevel::ERROR, std::string("JSON parse error: ") + e.what());
    throw;
  }
}

std::string stripDashes(const std::string &uuid) {
  std::string result;
  for (char c : uuid) {
    if (c != '-')
      result += c;
  }
  mc::log(mc::LogLevel::DEBUG, "Stripped UUID: " + result);
  return result;
}
