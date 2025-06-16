#pragma once

#include "packet.hpp"
#include "../util/buffer_utils.hpp"
#include "../network/socket.hpp"
#include <string>
#include <vector>

namespace mc {

class LoginDisconnect : public Packet {
public:
    std::string reason;

    LoginDisconnect() = default;

    std::vector<uint8_t> serialize() const override {
        return {};
    }

    uint32_t getPacketID() const override {
        return 0x00;
    }

    PacketDirection getDirection() const override {
        return mc::PacketDirection::Clientbound;    
    }

    void read(Socket& socket) {
        reason = socket.recvString();
    }
};

}

