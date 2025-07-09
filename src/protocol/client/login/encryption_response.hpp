#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <cstdint>
#include <vector>

namespace mc::protocol::client::login {

class EncryptionResponse : public Packet {
public:
  EncryptionResponse(const std::vector<uint8_t> &encryptedSecret,
                     const std::vector<uint8_t> &encryptedToken)
      : encryptedSecret_(encryptedSecret), encryptedToken_(encryptedToken) {}

  uint32_t getPacketID() const override { return 0x01; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeByteArray(encryptedSecret_);
    buf.writeByteArray(encryptedToken_);
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &buf) override {}

private:
  std::vector<uint8_t> encryptedSecret_;
  std::vector<uint8_t> encryptedToken_;
};

} // namespace mc::protocol::client::login
