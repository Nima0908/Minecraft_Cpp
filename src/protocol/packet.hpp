#pragma once
#include <vector>
#include <cstdint>

namespace mc {

class Packet {
public:
    virtual ~Packet() = default;

    virtual uint32_t getPacketID() const = 0;
    virtual std::vector<uint8_t> serialize() const = 0;
};

}
