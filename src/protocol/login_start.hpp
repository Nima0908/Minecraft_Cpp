#pragma once

#include "packet.hpp"
#include "../util/buffer_utils.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mc {
class LoginStart : public Packet {
public:
    std::string username;

    LoginStart(const std::string& user) : username(user) {}

    uint32_t getPacketID() const override {
        return 0x00;
    }

    PacketDirection getDirection() const override {
        return mc::PacketDirection::Serverbound;
    } 

    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;
        appendVarInt(data, getPacketID());
        appendString(data, username);

        std::vector<uint8_t> zeroUUID(16, 0);
        data.insert(data.end(), zeroUUID.begin(), zeroUUID.end());

        std::vector<uint8_t> fullPacket;
        appendVarInt(fullPacket, static_cast<int32_t>(data.size())); // Add length prefix
        fullPacket.insert(fullPacket.end(), data.begin(), data.end()); // Append body

        return fullPacket;
    }
};
}
