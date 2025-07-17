#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <string>

namespace mc::protocol::server::status {

class StatusResponse : public Packet {
public:
  StatusResponse() : json_("") {}

  uint32_t getPacketID() const override { return 0x00; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    return {};
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    json_ = buf.readString(); // Assumes VarInt-prefixed UTF-8 string
  }

private:
  std::string json_;
};

} // namespace mc::protocol::server::status
