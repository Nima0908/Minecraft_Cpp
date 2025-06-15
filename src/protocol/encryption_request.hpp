#pragma once

#include "packet.hpp"
#include "../util/buffer_utils.hpp"
#include "../network/socket.hpp"
#include <string>
#include <vector>

namespace mc {

class EncryptionRequest : public Packet {
public:
    std::string serverID;
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> verifyToken;

    EncryptionRequest() = default;

    // This packet is only received, not sent — serialization is not used.
    std::vector<uint8_t> serialize() const override {
        return {};  // Not applicable for received packets
    }

    uint32_t getPacketID() const override {
        return 0x01;
    }

    // Custom deserialization from live socket
    void read(Socket& socket) {
        serverID = socket.recvString();
        publicKey = socket.recvByteArray();
        verifyToken = socket.recvByteArray();
    }
};

}
