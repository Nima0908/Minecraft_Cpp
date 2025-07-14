#pragma once

#include "../../main.hpp"
#include "../../network/socket.hpp"

namespace mc::handler::stages {

class HandshakeHandler {
public:
  void sendHandshakeAndLogin(int32_t protocolVersion,
                             const ServerConnection &connection,
                             const std::string &minecraftUUID,
                             mc::network::Socket &socket);
};

} // namespace mc::handler::stages
