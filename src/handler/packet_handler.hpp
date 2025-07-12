#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>
#include <unordered_map>

#include "../buffer/write_buffer.hpp"
#include "../network/socket.hpp"
#include "../protocol/packet.hpp"
#include "../protocol/packet_state.hpp"

namespace mc::handler {

class PacketHandler {
public:
  explicit PacketHandler(mc::network::Socket &sock);
  ~PacketHandler();

  void startReceiving();
  void stopReceiving();

  void receivePacket(mc::network::Socket &socket);

  template <typename PacketType, typename... Args>
  void sendPacket(Args &&...args) {
    mc::buffer::WriteBuffer buf;
    PacketType packet(std::forward<Args>(args)...);
    socket.sendPacket(packet.serialize(buf));
  }

  mc::protocol::PacketState currentState =
      mc::protocol::PacketState::Handshaking;

private:
  void receiveLoop();

  mc::network::Socket &socket;
  std::thread receiveThread;
  std::atomic<bool> running{false};
};

} // namespace mc::handler
