#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <array>
#include <string>
#include <vector>

namespace mc::protocol::client::login {

class LoginStart : public Packet {
public:
  std::string username;
  std::array<uint8_t, 16> uuid;

  LoginStart() : username(""), uuid{} {}

  LoginStart(const std::string &user, const std::array<uint8_t, 16> &uuidBytes)
      : username(user), uuid(uuidBytes) {}

  uint32_t getPacketID() const override { return 0x00; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeString(username);
    buf.writeBytes(std::vector<uint8_t>(uuid.begin(), uuid.end()));
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &buf) override {}
};

} // namespace mc::protocol::client::login
