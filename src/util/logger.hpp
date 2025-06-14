#pragma once
#include <iostream>

namespace mc {

enum class LogLevel {
    INFO,
    WARN,
    ERROR,
    DEBUG
};

inline void log(LogLevel level, const std::string& message) {
    switch (level) {
        case LogLevel::INFO:  std::cout << "[INFO]  " << message << "\n"; break;
        case LogLevel::WARN:  std::cout << "[WARN]  " << message << "\n"; break;
        case LogLevel::ERROR: std::cerr << "[ERROR] " << message << "\n"; break;
        case LogLevel::DEBUG: std::cout << "[DEBUG] " << message << "\n"; break;
    }
}
}
