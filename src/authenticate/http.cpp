#include "http.hpp"
#include <curl/curl.h>
#include <stdexcept>

namespace mc::auth {

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

std::string httpPost(const std::string &url, const std::string &data,
                     const Headers &headers) {
  CURL *curl = curl_easy_init();
  if (!curl)
    throw std::runtime_error("curl init failed");

  std::string response;
  std::string errorBuffer(CURL_ERROR_SIZE, 0);

  struct curl_slist *chunk = nullptr;
  for (const auto &h : headers)
    chunk = curl_slist_append(chunk, h.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer.data());
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(chunk);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK)
    throw std::runtime_error("curl post error: " +
                             std::string(errorBuffer.c_str()));

  return response;
}

std::string httpGet(const std::string &url, const Headers &headers) {
  CURL *curl = curl_easy_init();
  if (!curl)
    throw std::runtime_error("curl init failed");

  std::string response;
  std::string errorBuffer(CURL_ERROR_SIZE, 0);

  struct curl_slist *chunk = nullptr;
  for (const auto &h : headers)
    chunk = curl_slist_append(chunk, h.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer.data());
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(chunk);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK)
    throw std::runtime_error("curl get error: " +
                             std::string(errorBuffer.c_str()));

  return response;
}
} // namespace mc::auth
