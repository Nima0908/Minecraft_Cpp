#include "../../../buffer/read_buffer.hpp"
#include "../../../buffer/write_buffer.hpp"
#include "../../packet.hpp"
namespace mc::protocol::client::handshaking {

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

  std::vector<uint8_t> serialize(mc::buffer::WriteBuffer &buf) const override {
    buf.writeVarInt(getPacketID());
    buf.writeVarInt(protocolVersion);
    buf.writeString(serverAddress);
    buf.writeUInt16(port);
    buf.writeVarInt(nextState);
    return buf.compile();
  }

  void read(mc::buffer::ReadBuffer &buf) override {};
};

} // namespace mc::protocol::client::handshaking
