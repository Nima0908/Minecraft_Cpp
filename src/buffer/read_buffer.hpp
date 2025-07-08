#pragma once
#include "types.hpp"
#include <cstddef>
#include <string.h>
#include <type_traits>
#include <stdexcept>

namespace mc::buffer {

class ReadBuffer {
private:
    ByteArray data_;
    size_t readPos_ = 0;

public:
    explicit ReadBuffer(ByteArray data);

    template<typename T>
    T read();

    bool ensure(size_t len) const;
    ByteArray readBytes(size_t len);
    uint8_t readByte();
    int32_t readVarInt();
    uint16_t readUInt16();
    std::string readString();
    ByteArray readByteArray();
    ByteArray copyRemaining() const;
    size_t remaining() const;
    const ByteArray& data() const;
};

// Template must remain in the header
template<typename T>
T ReadBuffer::read() {
    static_assert(std::is_trivially_copyable_v<T>, "Only trivial types supported");
    if (!ensure(sizeof(T))) throw std::runtime_error("Generic read out of bounds");
    T val;
    memcpy(&val, &data_[readPos_], sizeof(T));
    readPos_ += sizeof(T);
    return val;
}

} // namespace mc::buffer
