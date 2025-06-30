#pragma once

#include "../../network/socket.hpp"
#include "../../util/buffer_util.hpp"
#include "../packet.hpp"

namespace mc {

class LoginCompression : public Packet {
public:
  int threshold = 0;

  LoginCompression() = default;

  std::vector<uint8_t> serialize() const override { return {}; }

  uint32_t getPacketID() const override { return 0x03; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(BufferUtil &buf) { threshold = buf.readVarInt(); }
};

} // namespace mc
