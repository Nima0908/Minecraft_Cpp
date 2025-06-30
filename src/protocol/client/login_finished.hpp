#pragma once
#include "../../network/socket.hpp"
#include "../../util/buffer_util.hpp"
#include "../packet.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mc {

class LoginFinished : public Packet {
public:
  std::vector<uint8_t> uuid;
  std::string username;

  struct Property {
    std::string name;
    std::string value;
    std::optional<std::string> signature;
  };
  std::vector<Property> properties;

  LoginFinished() = default;

  std::vector<uint8_t> serialize() const override { return {}; }

  uint32_t getPacketID() const override { return 0x02; }

  PacketDirection getDirection() const override {
    return PacketDirection::Clientbound;
  }

  void read(BufferUtil &buf) {
    uuid = buf.readBytes(16);
    username = buf.readString();

    int count = buf.readVarInt();
    for (int i = 0; i < count; ++i) {
      Property p;
      p.name = buf.readString();
      p.value = buf.readString();

      if (buf.remaining() > 0) {
        if (uint8_t hasSig = buf.readByte(); hasSig) {
          p.signature = buf.readString();
        }
      }

      properties.push_back(p);
    }
  }
};

} // namespace mc
