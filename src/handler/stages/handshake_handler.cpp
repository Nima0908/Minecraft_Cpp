#include "handshake_handler.hpp"
#include "../../buffer/write_buffer.hpp"
#include "../../protocol/client/handshaking/handshake.hpp"
#include "../../protocol/client/login/login_start.hpp"
#include "../../util/logger.hpp"
#include "../../util/uuid_util.hpp"
#include "../packet_handler.hpp"

namespace mc::handler::stages {

void HandshakeHandler::sendHandshakeAndLogin(int32_t protocolVersion,
                                             const ServerConnection &connection,
                                             const std::string &minecraftUUID,
                                             mc::network::Socket &socket) {
  mc::handler::PacketHandler handler(socket);

  handler.sendPacket<mc::protocol::client::handshaking::HandshakePacket>(
      protocolVersion, connection.address, connection.port, 2);

  mc::utils::log(mc::utils::LogLevel::DEBUG, "Sent handshake packet");

  std::array<uint8_t, 16> uuidBytes = utils::parseDashlessUUID(minecraftUUID);

  handler.sendPacket<mc::protocol::client::login::LoginStart>(
      connection.username, uuidBytes);
  mc::utils::log(mc::utils::LogLevel::DEBUG, "Sent LoginStart packet");
}

} // namespace mc::handler::stages
