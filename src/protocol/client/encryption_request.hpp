#pragma once

#include "../packet.hpp"
#include "../../util/buffer_util.hpp"
#include "../../network/socket.hpp"
#include <string>
#include <vector>

namespace mc {

class EncryptionRequest : public Packet {
public:
    std::string serverID;
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> verifyToken;

    EncryptionRequest() = default;

    std::vector<uint8_t> serialize() const override {
        return {};  // Never sent
    }

    uint32_t getPacketID() const override {
        return 0x01;
    }

    PacketDirection getDirection() const override {
        return PacketDirection::Clientbound;
    }

    void read(Socket& socket) {
        serverID = socket.recvString();
        publicKey = socket.recvByteArray();
        verifyToken = socket.recvByteArray();
    }
};

}

