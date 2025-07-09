#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"

namespace mc::protocol::server::login {

class LoginCompression : public Packet {
public:
  int threshold = 0;

  LoginCompression() = default;

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    return {};
  }

  uint32_t getPacketID() const override { return 0x03; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    threshold = buf.readVarInt();
  }
};

} // namespace mc::protocol::server::login
