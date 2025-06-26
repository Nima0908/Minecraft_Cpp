#pragma once
#include "../packet.hpp"
#include "../../util/buffer_utils.hpp"
#include <string>

namespace mc {

class HandshakePacket : public Packet {
public:
    int32_t protocolVersion;
    std::string serverAddress;
    uint16_t serverPort;
    int32_t nextState;

    HandshakePacket(int32_t version, std::string address, uint16_t port, int32_t state)
        : protocolVersion(version), serverAddress(std::move(address)), serverPort(port), nextState(state) {}

    uint32_t getPacketID() const override {
        return 0x00;
    }

    PacketDirection getDirection() const override {
        return mc::PacketDirection::Serverbound;
    }

    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;

        appendVarInt(data, getPacketID());
        appendVarInt(data, protocolVersion);
        appendString(data, serverAddress);
        appendUInt16(data, serverPort);
        appendVarInt(data, nextState);
        
        std::vector<uint8_t> fullPacket;
        appendVarInt(fullPacket, static_cast<int32_t>(data.size())); // Add length prefix
        fullPacket.insert(fullPacket.end(), data.begin(), data.end()); // Append body

    return fullPacket;
    }
};

}
