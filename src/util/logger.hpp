#pragma once

#include "log_level.hpp"
#include <iostream>

namespace mc::utils {

#if defined(MC_ENABLE_LOGGING)

inline constexpr const char *RESET  = "\033[0m";
inline constexpr const char *RED    = "\033[31m";
inline constexpr const char *YELLOW = "\033[33m";
inline constexpr const char *GREEN  = "\033[32m";
inline constexpr const char *CYAN   = "\033[36m";

template <typename... Args>
void log(LogLevel level, Args&&... args) {
  std::ostream* out = &std::cout;
  const char* color = "";
  const char* prefix = "";

  switch (level) {
    case LogLevel::INFO:
      color = GREEN; prefix = "[INFO]  "; break;
    case LogLevel::WARN:
      color = YELLOW; prefix = "[WARN]  "; break;
    case LogLevel::ERROR:
      out = &std::cerr; color = RED; prefix = "[ERROR] "; break;
    case LogLevel::DEBUG:
      color = CYAN; prefix = "[DEBUG] "; break;
  }

  *out << color << prefix << RESET;
  ((*out << std::forward<Args>(args)), ...) << '\n';
}

#else

template <typename... Args>
inline void log(LogLevel, Args&&...) {}

#endif

} // namespace mc::utils
