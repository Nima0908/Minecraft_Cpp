#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <string>
#include <vector>

namespace mc::protocol::server::configuration {

class Ping : public Packet {
public:
  int id_;

  Ping() = default;

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    return {};
  }

  uint32_t getPacketID() const override { return 0x05; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(mc::buffer::ReadBuffer &buf) override { id_ = buf.readInt32(); }
};

} // namespace mc::protocol::server::configuration
