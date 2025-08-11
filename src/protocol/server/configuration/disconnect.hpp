#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../../datatypes/text_component/text_component.hpp"
#include "../../packet.hpp"

#include <vector>

namespace mc::protocol::server::configuration {

class Disconnect : public Packet {
public:
  mc::datatypes::text_component::TextComponent reason;

  Disconnect() = default;

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    return {};
  }

  uint32_t getPacketID() const override { return 0x02; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(mc::buffer::ReadBuffer &buf) override { reason.deserialize(buf); }
};

} // namespace mc::protocol::server::configuration
