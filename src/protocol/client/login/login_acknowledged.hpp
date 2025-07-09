#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <vector>

namespace mc::protocol::client::login {

class LoginAcknowledged : public Packet {
public:
  LoginAcknowledged() = default;

  uint32_t getPacketID() const override { return 0x03; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &buf) override {}
};

} // namespace mc::protocol::client::login
