#pragma once
#include <optional>
#include <string>

std::optional<std::string> loadToken(const std::string &filename);
void saveToken(const std::string &filename, const std::string &token);
void clearToken(const std::string &filename);
