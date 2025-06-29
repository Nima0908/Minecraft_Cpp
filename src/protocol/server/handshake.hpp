#include "../../util/buffer_util.hpp"

namespace mc {

class HandshakePacket {
public:
    int32_t protocolVersion;
    std::string serverAddress;
    uint16_t port;
    int32_t nextState;

    HandshakePacket(int32_t protocol, const std::string& addr, uint16_t p, int32_t state)
        : protocolVersion(protocol), serverAddress(addr), port(p), nextState(state) {}

    std::vector<uint8_t> serialize() const {
        BufferUtil buf;
        buf.writeVarInt(0x00); // Packet ID
        buf.writeVarInt(protocolVersion);
        buf.writeString(serverAddress);
        buf.writeUInt16(port);
        buf.writeVarInt(nextState);
        return buf.data();
    }
};

} // namespace mc
