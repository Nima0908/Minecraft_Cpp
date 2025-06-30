#include "token_cache.hpp"
#include <fstream>

std::optional<std::string> loadToken(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open())
    return std::nullopt;

  std::string token;
  std::getline(file, token);
  return token.empty() ? std::nullopt : std::optional<std::string>(token);
}

void saveToken(const std::string &filename, const std::string &token) {
  std::ofstream file(filename, std::ios::trunc);
  if (!file.is_open())
    throw std::runtime_error("Cannot open token file for writing");
  file << token;
}

void clearToken(const std::string &filename) { std::remove(filename.c_str()); }
