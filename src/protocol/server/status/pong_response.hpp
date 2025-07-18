#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>

namespace mc::protocol::server::status {

class PongResponse : public Packet {
public:
  PongResponse() : timestamp_(0) {}

  int64_t timestamp_;

  uint32_t getPacketID() const override { return 0x01; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    return {};
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    timestamp_ = buf.readLong(); // Assumes 64-bit signed integer
  }
};

} // namespace mc::protocol::server::status
