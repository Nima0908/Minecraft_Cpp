#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mc::protocol::client::login {

class CookieResponse : public Packet {
public:
  CookieResponse() : key_(""), payload_(std::nullopt) {}

  CookieResponse(const std::string &key,
                 const std::optional<std::vector<uint8_t>> &payload)
      : key_(key), payload_(payload) {}

  uint32_t getPacketID() const override { return 0x04; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeString(key_);
    buf.writeByteArray(payload_.value());
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    key_ = buf.readString();
    payload_ = buf.readByteArray();
  }

private:
  std::string key_;
  std::optional<std::vector<uint8_t>> payload_;
};

} // namespace mc::protocol::client::login
