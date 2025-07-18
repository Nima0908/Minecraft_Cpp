#pragma once

#include "../../buffer/read_buffer.hpp"
#include "../../network/socket.hpp"
#include "../../protocol/packet.hpp"
#include "../../protocol/server/status/pong_response.hpp"
#include "../../protocol/server/status/status_response.hpp"

namespace mc::handler::stages {

class StatusHandler {
public:
  void determinePacket(std::unique_ptr<mc::protocol::Packet> &packet,
                       mc::buffer::ReadBuffer &read);

private:
  void
  handlePongResponse(mc::protocol::server::status::PongResponse &pongResponse,
                     mc::buffer::ReadBuffer &read);

  void handleStatusResponse(
      mc::protocol::server::status::StatusResponse &statusResponse,
      mc::buffer::ReadBuffer &read);
};

} // namespace mc::handler::stages
