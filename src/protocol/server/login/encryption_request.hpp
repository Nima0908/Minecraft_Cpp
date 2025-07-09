#pragma once

#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
#include <string>
#include <vector>

namespace mc::protocol::server::login {

class EncryptionRequest : public Packet {
public:
  std::string serverID;
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> verifyToken;

  EncryptionRequest() = default;

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    return {};
  }

  uint32_t getPacketID() const override { return 0x01; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(mc::buffer::ReadBuffer &buf) override {
    serverID = buf.readString();
    publicKey = buf.readByteArray();
    verifyToken = buf.readByteArray();
  }
};

} // namespace mc::protocol::server::login
