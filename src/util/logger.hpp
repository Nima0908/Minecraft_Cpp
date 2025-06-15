#pragma once
#include <iostream>

namespace mc {

enum class LogLevel {
    INFO,
    WARN,
    ERROR,
    DEBUG
};

inline constexpr const char* RESET   = "\033[0m";
inline constexpr const char* RED     = "\033[31m";
inline constexpr const char* YELLOW  = "\033[33m";
inline constexpr const char* GREEN   = "\033[32m";
inline constexpr const char* CYAN    = "\033[36m";

inline void log(LogLevel level, const std::string& message) {
    switch (level) {
        case LogLevel::INFO:
            std::cout << GREEN << "[INFO]  " << RESET << message << "\n";
            break;
        case LogLevel::WARN:
            std::cout << YELLOW << "[WARN]  " << RESET << message << "\n";
            break;
        case LogLevel::ERROR:
            std::cerr << RED << "[ERROR] " << RESET << message << "\n";
            break;
        case LogLevel::DEBUG:
            std::cout << CYAN << "[DEBUG] " << RESET << message << "\n";
            break;
    }
}

}

