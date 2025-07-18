#pragma once

#include "../protocol/packet.hpp"
#include "logger.hpp"
#include <memory>
#include <string>

namespace mc::utils {

template <typename T>
T *tryCastPacket(std::unique_ptr<mc::protocol::Packet> &packet,
                 const std::string &typeName) {
  if (auto *casted = dynamic_cast<T *>(packet.get())) {
    return casted;
  }
  mc::utils::log(mc::utils::LogLevel::ERROR,
                 "Invalid packet cast to " + typeName);
  return nullptr;
}

} // namespace mc::utils
