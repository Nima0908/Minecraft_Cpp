#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <string>
#include <vector>

namespace mc::protocol::server::configuration {

class KeepAlive : public Packet {
public:
  long keepAliveId_;

  KeepAlive() = default;

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    return {};
  }

  uint32_t getPacketID() const override { return 0x04; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    keepAliveId_ = buf.readLong();
  }
};

} // namespace mc::protocol::server::configuration
