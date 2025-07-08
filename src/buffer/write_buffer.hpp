#pragma once
#include "types.hpp"
#include <vector>
#include <cstddef>
#include <type_traits>

namespace mc::buffer {

class WriteBuffer {
private:
    std::vector<ByteArray> segments_;
    size_t totalSize_ = 0;

public:
    WriteBuffer();

    template<typename T>
    void write(const T& value);

    template<typename T>
    void write(const std::vector<T>& values);

    void writeBytes(const ByteArray& data);
    void writeRaw(const void* data, size_t size);
    ByteArray compile() const;
    void clear();

    // Primitives
    void writeVarInt(int32_t value);
    void writeUInt16(uint16_t value);
    void writeString(const std::string& str);
    void writeByteArray(const ByteArray& bytes);
};

// Template implementations must go in the header!
template<typename T>
void WriteBuffer::write(const T& value) {
    static_assert(std::is_trivially_copyable_v<T>, "Only trivial types supported");
    writeRaw(&value, sizeof(T));
}

template<typename T>
void WriteBuffer::write(const std::vector<T>& values) {
    for (const auto& v : values) write(v);
}

} // namespace mc::buffer
