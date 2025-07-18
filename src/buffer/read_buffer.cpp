#include "read_buffer.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace mc::buffer {

ReadBuffer::ReadBuffer(ByteArray data) : data_(std::move(data)), readPos_(0) {}

bool ReadBuffer::ensure(size_t len) const {
  return readPos_ + len <= data_.size();
}

ByteArray ReadBuffer::readBytes(size_t len) {
  if (!ensure(len))
    throw std::runtime_error("Read out of bounds");
  ByteArray out(data_.begin() + readPos_, data_.begin() + readPos_ + len);
  readPos_ += len;
  return out;
}

uint8_t ReadBuffer::readByte() {
  if (!ensure(1))
    throw std::runtime_error("Byte read out of bounds");
  return data_[readPos_++];
}

int8_t ReadBuffer::readInt8() {
  if (!ensure(1))
    throw std::runtime_error("Int8 read out of bounds");
  return static_cast<int8_t>(data_[readPos_++]);
}

bool ReadBuffer::readBool() { return readByte() != 0; }

int16_t ReadBuffer::readInt16() {
  if (!ensure(2))
    throw std::runtime_error("Int16 read out of bounds");
  int16_t val = (data_[readPos_] << 8) | data_[readPos_ + 1];
  readPos_ += 2;
  return val;
}

int32_t ReadBuffer::readInt32() {
  if (!ensure(4))
    throw std::runtime_error("Int32 read out of bounds");
  int32_t val = 0;
  for (int i = 0; i < 4; ++i) {
    val = (val << 8) | data_[readPos_++];
  }
  return val;
}

uint16_t ReadBuffer::readUInt16() {
  if (!ensure(2))
    throw std::runtime_error("UInt16 read out of bounds");
  uint16_t val = (data_[readPos_] << 8) | data_[readPos_ + 1];
  readPos_ += 2;
  return val;
}

uint32_t ReadBuffer::readUInt32() {
  if (!ensure(4))
    throw std::runtime_error("UInt32 read out of bounds");
  uint32_t val = 0;
  for (int i = 0; i < 4; ++i) {
    val = (val << 8) | data_[readPos_++];
  }
  return val;
}

int32_t ReadBuffer::readVarInt() {
  int32_t result = 0;
  int numRead = 0;
  uint8_t byte;

  do {
    if (!ensure(1))
      throw std::runtime_error("VarInt read out of bounds");
    byte = data_[readPos_++];
    result |= (byte & 0x7F) << (7 * numRead);
    if (++numRead > 5)
      throw std::runtime_error("VarInt too big");
  } while (byte & 0x80);

  return result;
}

int64_t ReadBuffer::readLong() {
  if (!ensure(8))
    throw std::runtime_error("Long read out of bounds");

  int64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value <<= 8;
    value |= static_cast<uint8_t>(data_[readPos_++]);
  }
  return value;
}

float ReadBuffer::readFloat() { return read<float>(); }

double ReadBuffer::readDouble() { return read<double>(); }

std::string ReadBuffer::readString() {
  int len = readVarInt();
  if (!ensure(len))
    throw std::runtime_error("String read out of bounds");
  std::string str(data_.begin() + readPos_, data_.begin() + readPos_ + len);
  readPos_ += len;
  return str;
}

ByteArray ReadBuffer::readByteArray() {
  int len = readVarInt();
  return readBytes(len);
}

ByteArray ReadBuffer::copyRemaining() const {
  return ByteArray(data_.begin() + readPos_, data_.end());
}

size_t ReadBuffer::remaining() const { return data_.size() - readPos_; }

const ByteArray &ReadBuffer::data() const { return data_; }

} // namespace mc::buffer
