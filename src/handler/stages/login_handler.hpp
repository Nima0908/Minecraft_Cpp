#pragma once

#include "../../network/socket.hpp"
#include "../../protocol/packet.hpp"

namespace mc::handler::stages {

class LoginHandler {
public:
  // Determines what type of packet is received and dispatches it to appropriate
  // handler
  void determinePacket(mc::network::Socket &socket,
                       std::unique_ptr<mc::protocol::Packet> &packet,
                       const std::string &mcToken,
                       const std::string &minecraftUUID);

  // Specific handlers for expected login packets
  void handleLoginAcknowledged(std::unique_ptr<mc::protocol::Packet> &packet);
  void handleEncryptionResponse(mc::network::Socket &socket,
                                std::unique_ptr<mc::protocol::Packet> &packet,
                                const std::string &mcToken,
                                const std::string &minecraftUUID);

  void handleLoginCompression(mc::network::Socket &socket,
                              std::unique_ptr<mc::protocol::Packet> &packet);
  void handleLoginDisconnect(mc::network::Socket &socket,
                             std::unique_ptr<mc::protocol::Packet> &packet);
};
} // namespace mc::handler::stages
