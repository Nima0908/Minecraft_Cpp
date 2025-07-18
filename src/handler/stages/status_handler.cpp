#include "status_handler.hpp"

#include "../../buffer/read_buffer.hpp"
#include "../../protocol/packet.hpp"
#include "../../protocol/server/status/pong_response.hpp"
#include "../../protocol/server/status/status_response.hpp"
#include "../../util/logger.hpp"

#include <memory>
#include <sstream>
#include <string>

namespace mc::handler::stages {

void StatusHandler::determinePacket(
    std::unique_ptr<mc::protocol::Packet> &packet,
    mc::buffer::ReadBuffer &read) {
  using namespace mc::protocol::server::status;

  if (auto *statusResponse = dynamic_cast<StatusResponse *>(packet.get())) {
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Handling StatusResponse packet");
    handleStatusResponse(*statusResponse, read);
  } else if (auto *pongResponse = dynamic_cast<PongResponse *>(packet.get())) {
    mc::utils::log(mc::utils::LogLevel::DEBUG, "Handling PongResponse packet");
    handlePongResponse(*pongResponse, read);
  } else {
    mc::utils::log(mc::utils::LogLevel::WARN, "Unknown packet received");
  }
}

void StatusHandler::handlePongResponse(
    protocol::server::status::PongResponse &pongResponse,
    mc::buffer::ReadBuffer &read) {
  pongResponse.read(read);
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Ping took: " + std::to_string(pongResponse.timestamp_) +
                     "ms");
}

void StatusHandler::handleStatusResponse(
    protocol::server::status::StatusResponse &statusResponse,
    mc::buffer::ReadBuffer &read) {
  statusResponse.read(read);
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Status information: " + statusResponse.json_);
}

} // namespace mc::handler::stages
