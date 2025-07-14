#pragma once

#include "../main.hpp" // For ServerConnection
#include "../network/socket.hpp"
#include "../protocol/packet.hpp"
#include "../protocol/packet_direction.hpp"
#include "../protocol/packet_state.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace mc::handler {

class PacketHandler {
public:
  explicit PacketHandler(mc::network::Socket &sock);
  ~PacketHandler();

  void initialize(const ServerConnection &conn, const std::string &uuid);

  void startReceiving();
  void stopReceiving();
  void receivePacket();

  template <typename PacketType, typename... Args>
  void sendPacket(Args &&...args) {
    mc::buffer::WriteBuffer buf;
    PacketType packet(std::forward<Args>(args)...);
    socket.sendPacket(packet.serialize(buf));
  }

private:
  // === Connection Context ===
  mc::network::Socket &socket;
  ServerConnection connection;
  std::string minecraftUUID;

  // === Threading ===
  std::thread receiveThread;
  std::atomic<bool> running{false};

  // === State ===
  mc::protocol::PacketState currentState =
      mc::protocol::PacketState::Handshaking;

  // === Internals ===
  void receiveLoop();
  void handleHandshake();
  void handleGenericPacket();

  std::unique_ptr<mc::protocol::Packet>
  createPacketFromId(mc::protocol::PacketState state,
                     mc::protocol::PacketDirection direction, uint8_t packetId);
};

} // namespace mc::handler
