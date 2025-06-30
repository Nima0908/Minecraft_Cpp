#include "../../util/buffer_util.hpp"
#include "../packet.hpp"
namespace mc {

class HandshakePacket : public Packet {
public:
  int32_t protocolVersion;
  std::string serverAddress;
  uint16_t port;
  int32_t nextState;

  HandshakePacket(int32_t protocol, const std::string &addr, uint16_t p,
                  int32_t state)
      : protocolVersion(protocol), serverAddress(addr), port(p),
        nextState(state) {}

  uint32_t getPacketID() const override { return 0x00; }

  PacketDirection getDirection() const override {
    return PacketDirection::Serverbound;
  }

  std::vector<uint8_t> serialize() const override {
    BufferUtil buf;
    buf.writeVarInt(getPacketID());
    buf.writeVarInt(protocolVersion);
    buf.writeString(serverAddress);
    buf.writeUInt16(port);
    buf.writeVarInt(nextState);
    return buf.data();
  }
};

} // namespace mc
