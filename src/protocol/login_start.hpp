#pragma once

#include "packet.hpp"
#include "../util/buffer_utils.hpp"

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

    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;
        appendVarInt(data, getPacketID());
        appendString(data, username);
        return data;
    }
};
}
