#pragma once

#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <vector>

namespace mc::protocol::client::status {

class PingRequest : public Packet {
public:
  PingRequest() : timestamp_(0) {}
  explicit PingRequest(int64_t timestamp) : timestamp_(timestamp) {}

  uint32_t getPacketID() const override { return 0x01; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeLong(timestamp_);
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &) override {
    // Not used for serverbound packet
  }

private:
  int64_t timestamp_;
};
} // namespace mc::protocol::client::status
