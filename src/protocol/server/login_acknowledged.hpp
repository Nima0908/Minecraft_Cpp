#pragma once

#include "../../util/buffer_util.hpp"
#include "../packet.hpp"
#include <array>
#include <string>
#include <vector>

namespace mc {

class LoginAcknowledged : public Packet {
public:
  LoginAcknowledged() = default;

  uint32_t getPacketID() const override { return 0x03; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize() const override {
    BufferUtil buf;
    buf.writeVarInt(getPacketID());
    return buf.data();
  }
};

} // namespace mc
