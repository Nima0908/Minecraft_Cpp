#pragma once

#include "../../network/socket.hpp"
#include "../../protocol/packet.hpp"
#include "../../protocol/server/login/cookie_request.hpp"
#include "../../protocol/server/login/custom_query.hpp"
#include "../../protocol/server/login/encryption_request.hpp"
#include "../../protocol/server/login/login_compression.hpp"
#include "../../protocol/server/login/login_disconnect.hpp"
#include "../../protocol/server/login/login_finished.hpp"
#include <memory>
#include <string>

namespace mc::handler::stages {

class LoginHandler {
public:
  void determinePacket(mc::network::Socket &socket,
                       std::unique_ptr<mc::protocol::Packet> &packet,
                       const std::string &mcToken,
                       const std::string &minecraftUUID);

private:
  void handleEncryptionResponse(
      mc::network::Socket &socket,
      mc::protocol::server::login::EncryptionRequest &encryptionRequest,
      const std::string &mcToken, const std::string &minecraftUUID);

  void handleLoginCompression(
      mc::network::Socket &socket,
      mc::protocol::server::login::LoginCompression &loginCompression);

  void handleLoginAcknowledged(
      mc::protocol::server::login::LoginFinished &loginSuccess);

  void handleLoginDisconnect(
      mc::network::Socket &socket,
      mc::protocol::server::login::LoginDisconnect &loginDisconnect);
};

} // namespace mc::handler::stages
