#pragma once

#include "../../util/buffer_util.hpp"
#include "../packet.hpp"
#include <string>
#include <vector>

namespace mc::protocol::client {

class EncryptionRequest : public Packet {
public:
  std::string serverID;
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> verifyToken;

  EncryptionRequest() = default;

  std::vector<uint8_t> serialize() const override { return {}; }

  uint32_t getPacketID() const override { return 0x01; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(mc::utils::BufferUtil &buf) {
    serverID = buf.readString();
    publicKey = buf.readByteArray();
    verifyToken = buf.readByteArray();
  }
};

} // namespace mc::protocol::client
