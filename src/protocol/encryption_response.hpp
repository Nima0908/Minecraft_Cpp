#pragma once

#include "packet.hpp"
#include "../util/buffer_utils.hpp"
#include <vector>
#include <cstdint>

namespace mc {

class EncryptionResponse : public Packet {
public:
    EncryptionResponse(const std::vector<uint8_t>& encryptedSecret,
                             const std::vector<uint8_t>& encryptedToken)
        : encryptedSecret_(encryptedSecret), encryptedToken_(encryptedToken) {}

    uint32_t getPacketID() const override {
        return 0x01;
    }

    PacketDirection getDirection() const override {
        return mc::PacketDirection::Serverbound;
  }

    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;
        appendVarInt(data, getPacketID());
        appendByteArray(data, encryptedSecret_);
        appendByteArray(data, encryptedToken_);
        return data;
    }

private:
    std::vector<uint8_t> encryptedSecret_;
    std::vector<uint8_t> encryptedToken_;
};

}
