#pragma once
#include <vector>
#include <cstdint>

namespace mc {

enum class PacketDirection { Clientbound, Serverbound };

class Packet {
public:
    virtual ~Packet() = default;

    virtual uint32_t getPacketID() const = 0;
    virtual PacketDirection getDirection() const = 0;
    virtual std::vector<uint8_t> serialize() const = 0;
};

}
