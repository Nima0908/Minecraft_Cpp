#pragma once
#include "../buffer/types.hpp"
#include <cstring>
#include <type_traits>

namespace mc::utils {

// Fast serialization utilities with minimal overhead
class FastSerializer {
public:
  explicit FastSerializer(mc::buffer::ByteArray &buffer) : buffer_(buffer) {}

  // Fast integer serialization using bit manipulation
  template <typename T> void writeIntBE(T value) {
    static_assert(std::is_integral_v<T>, "T must be integral");
    constexpr size_t size = sizeof(T);

    buffer_.reserve(buffer_.size() + size);

    if constexpr (size == 1) {
      buffer_.push_back(static_cast<uint8_t>(value));
    } else if constexpr (size == 2) {
      buffer_.push_back(static_cast<uint8_t>(value >> 8));
      buffer_.push_back(static_cast<uint8_t>(value));
    } else if constexpr (size == 4) {
      buffer_.push_back(static_cast<uint8_t>(value >> 24));
      buffer_.push_back(static_cast<uint8_t>(value >> 16));
      buffer_.push_back(static_cast<uint8_t>(value >> 8));
      buffer_.push_back(static_cast<uint8_t>(value));
    } else if constexpr (size == 8) {
      buffer_.push_back(static_cast<uint8_t>(value >> 56));
      buffer_.push_back(static_cast<uint8_t>(value >> 48));
      buffer_.push_back(static_cast<uint8_t>(value >> 40));
      buffer_.push_back(static_cast<uint8_t>(value >> 32));
      buffer_.push_back(static_cast<uint8_t>(value >> 24));
      buffer_.push_back(static_cast<uint8_t>(value >> 16));
      buffer_.push_back(static_cast<uint8_t>(value >> 8));
      buffer_.push_back(static_cast<uint8_t>(value));
    }
  }

  // Optimized VarInt encoding
  void writeVarInt(int32_t value) {
    uint32_t uvalue = static_cast<uint32_t>(value);

    while (uvalue >= 0x80) {
      buffer_.push_back(static_cast<uint8_t>(uvalue | 0x80));
      uvalue >>= 7;
    }
    buffer_.push_back(static_cast<uint8_t>(uvalue));
  }

  // Fast string writing
  void writeString(const std::string &str) {
    writeVarInt(static_cast<int32_t>(str.size()));
    buffer_.insert(buffer_.end(), str.begin(), str.end());
  }

  // Raw data copy
  void writeRaw(const void *data, size_t size) {
    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    buffer_.insert(buffer_.end(), bytes, bytes + size);
  }

private:
  mc::buffer::ByteArray &buffer_;
};

// Fast deserialization utilities
class FastDeserializer {
public:
  explicit FastDeserializer(const mc::buffer::ByteArray &buffer)
      : buffer_(buffer), pos_(0) {}

  template <typename T> T readIntBE() {
    static_assert(std::is_integral_v<T>, "T must be integral");
    constexpr size_t size = sizeof(T);

    if (pos_ + size > buffer_.size()) {
      throw std::runtime_error("Read out of bounds");
    }

    T value = 0;
    if constexpr (size == 1) {
      value = static_cast<T>(buffer_[pos_]);
    } else if constexpr (size == 2) {
      value = (static_cast<T>(buffer_[pos_]) << 8) |
              static_cast<T>(buffer_[pos_ + 1]);
    } else if constexpr (size == 4) {
      value = (static_cast<T>(buffer_[pos_]) << 24) |
              (static_cast<T>(buffer_[pos_ + 1]) << 16) |
              (static_cast<T>(buffer_[pos_ + 2]) << 8) |
              static_cast<T>(buffer_[pos_ + 3]);
    } else if constexpr (size == 8) {
      value = (static_cast<T>(buffer_[pos_]) << 56) |
              (static_cast<T>(buffer_[pos_ + 1]) << 48) |
              (static_cast<T>(buffer_[pos_ + 2]) << 40) |
              (static_cast<T>(buffer_[pos_ + 3]) << 32) |
              (static_cast<T>(buffer_[pos_ + 4]) << 24) |
              (static_cast<T>(buffer_[pos_ + 5]) << 16) |
              (static_cast<T>(buffer_[pos_ + 6]) << 8) |
              static_cast<T>(buffer_[pos_ + 7]);
    }

    pos_ += size;
    return value;
  }

  int32_t readVarInt() {
    int32_t result = 0;
    int shift = 0;

    while (pos_ < buffer_.size()) {
      uint8_t byte = buffer_[pos_++];
      result |= (byte & 0x7F) << shift;

      if ((byte & 0x80) == 0) {
        break;
      }

      shift += 7;
      if (shift >= 32) {
        throw std::runtime_error("VarInt too big");
      }
    }

    return result;
  }

  std::string readString() {
    int32_t length = readVarInt();
    if (pos_ + length > buffer_.size()) {
      throw std::runtime_error("String read out of bounds");
    }

    std::string result(buffer_.begin() + pos_, buffer_.begin() + pos_ + length);
    pos_ += length;
    return result;
  }

  size_t remaining() const { return buffer_.size() - pos_; }

private:
  const mc::buffer::ByteArray &buffer_;
  size_t pos_;
};

} // namespace mc::utils