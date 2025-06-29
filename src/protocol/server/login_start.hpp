#pragma once

#include "../packet.hpp"
#include "../../util/buffer_util.hpp"
#include <array>
#include <string>
#include <vector>

namespace mc {

class LoginStart : public Packet {
public:
    std::string username;
    std::array<uint8_t, 16> uuid;

    LoginStart(const std::string& user, const std::array<uint8_t, 16>& uuidBytes)
        : username(user), uuid(uuidBytes) {}

    uint32_t getPacketID() const override {
        return 0x00;
    }

    PacketDirection getDirection() const override {
        return PacketDirection::Serverbound;
    }

    std::vector<uint8_t> serialize() const override {
        BufferUtil buf;
        buf.writeVarInt(getPacketID());
        buf.writeString(username);
        buf.writeBytes(std::vector<uint8_t>(uuid.begin(), uuid.end()));
        return buf.data();
    }
};

}

