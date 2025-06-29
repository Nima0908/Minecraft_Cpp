#pragma once

#include "../packet.hpp"
#include "../../network/socket.hpp"

namespace mc {

class LoginCompression : public Packet {
public:
    int threshold = 0;

    LoginCompression() = default;

    std::vector<uint8_t> serialize() const override {
        return {};
    }

    uint32_t getPacketID() const override {
        return 0x03;
    }

    PacketDirection getDirection() const override {
        return PacketDirection::Clientbound;
    }

    void read(Socket& socket) {
        threshold = socket.recvVarInt();
    }
};

}
