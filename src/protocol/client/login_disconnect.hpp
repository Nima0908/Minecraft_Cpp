#pragma once

#include "../../util/buffer_util.hpp"
#include "../packet.hpp"
#include <string>
#include <vector>

namespace mc::protocol::client {

class LoginDisconnect : public Packet {
public:
  std::string reason;

  LoginDisconnect() = default;

  std::vector<uint8_t> serialize() const override { return {}; }

  uint32_t getPacketID() const override { return 0x00; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(mc::utils::BufferUtil &buf) { reason = buf.readString(); }
};

} // namespace mc::protocol::client
