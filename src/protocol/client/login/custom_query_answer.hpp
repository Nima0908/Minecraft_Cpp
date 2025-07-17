#pragma once

#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace mc::protocol::client::login {

class CustomQueryAnswer : public Packet {
public:
  CustomQueryAnswer() : messageID_(0), data_(std::nullopt) {}

  CustomQueryAnswer(int32_t messageID,
                    const std::optional<std::vector<uint8_t>> &data)
      : messageID_(messageID), data_(data) {}

  uint32_t getPacketID() const override { return 0x02; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeVarInt(messageID_);
    buf.writeByteArray(data_.value());
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &) override {
    // Not used — serverbound packets don't implement `read()`
  }

private:
  int32_t messageID_;
  std::optional<std::vector<uint8_t>> data_;
};

} // namespace mc::protocol::client::login
