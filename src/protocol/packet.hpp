#pragma once
#include "../buffer/write_buffer.hpp"
#include <cstdint>
#include <vector>

namespace mc::protocol {

enum class PacketDirection { Clientbound, Serverbound };

class Packet {
public:
  virtual ~Packet() = default;

  virtual uint32_t getPacketID() const = 0;
  virtual PacketDirection getDirection() const = 0;
  virtual std::vector<uint8_t>
  serialize(mc::buffer::WriteBuffer &buf) const = 0;
  virtual void read(mc::buffer::ReadBuffer &buf) = 0;
};

} // namespace mc::protocol
