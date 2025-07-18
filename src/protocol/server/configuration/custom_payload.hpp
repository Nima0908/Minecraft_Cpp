#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace mc::protocol::client::configuration {

class CustomPayload : public Packet {
public:
  CustomPayload() : channel_(""), data_() {}

  CustomPayload(const std::string &channel, const std::vector<uint8_t> &data)
      : channel_(channel), data_(data) {}

  std::string channel_;
  std::vector<uint8_t> data_;

  uint32_t getPacketID() const override { return 0x01; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeString(channel_);
    buf.writeByteArray(data_);
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    channel_ = buf.readString();

    // Calculate how many bytes are left in the buffer
    std::size_t bytesRemaining =
        buf.remaining(); // You may need to implement or adjust this
    data_.resize(bytesRemaining);
    for (std::size_t i = 0; i < bytesRemaining; ++i) {
      data_[i] = buf.readByte();
    }
  }
};

} // namespace mc::protocol::client::configuration
