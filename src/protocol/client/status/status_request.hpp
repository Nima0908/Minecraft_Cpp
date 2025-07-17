#pragma once

#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <vector>

namespace mc::protocol::client::status {

class StatusRequest : public Packet {
public:
  StatusRequest() = default;

  uint32_t getPacketID() const override { return 0x00; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    // no fields to write
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &) override {
    // Not used for serverbound packet
  }
};

} // namespace mc::protocol::client::status
