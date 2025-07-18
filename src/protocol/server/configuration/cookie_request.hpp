#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <string>

namespace mc::protocol::client::configuration {

class CookieRequest : public Packet {
public:
  CookieRequest() : identifier_("") {}

  explicit CookieRequest(const std::string &identifier)
      : identifier_(identifier) {}

  std::string identifier_;

  uint32_t getPacketID() const override { return 0x00; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeString(identifier_);
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    identifier_ = buf.readString();
  }
};

} // namespace mc::protocol::client::configuration
