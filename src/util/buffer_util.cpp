#include "buffer_util.hpp"
#include "logger.hpp"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace mc {

BufferUtil::BufferUtil() : readPos_(0) {}

BufferUtil::BufferUtil(std::vector<uint8_t> data)
    : buffer_(std::move(data)), readPos_(0) {}

void BufferUtil::writeVarInt(int32_t value) {
  do {
    uint8_t temp = value & 0x7F;
    value >>= 7;
    if (value != 0)
      temp |= 0x80;
    buffer_.push_back(temp);
  } while (value != 0);
}

void BufferUtil::writeUInt16(uint16_t value) {
  buffer_.push_back((value >> 8) & 0xFF);
  buffer_.push_back(value & 0xFF);
}

void BufferUtil::writeString(const std::string &str) {
  writeVarInt(static_cast<int32_t>(str.size()));
  buffer_.insert(buffer_.end(), str.begin(), str.end());
}

void BufferUtil::writeByteArray(const std::vector<uint8_t> &bytes) {
  writeVarInt(static_cast<int32_t>(bytes.size()));
  buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

void BufferUtil::writeBytes(const std::vector<uint8_t> &bytes) {
  buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

int32_t BufferUtil::readVarInt() {
  int32_t result = 0;
  int numRead = 0;
  uint8_t byte;
  do {
    if (readPos_ >= buffer_.size())
      throw std::runtime_error("VarInt read out of bounds");
    byte = buffer_[readPos_++];
    result |= (byte & 0x7F) << (7 * numRead);
    if (++numRead > 5)
      throw std::runtime_error("VarInt too big");
  } while (byte & 0x80);
  return result;
}

uint16_t BufferUtil::readUInt16() {
  if (readPos_ + 2 > buffer_.size())
    throw std::runtime_error("UInt16 read out of bounds");
  uint16_t value = (buffer_[readPos_] << 8) | buffer_[readPos_ + 1];
  readPos_ += 2;
  return value;
}

uint8_t BufferUtil::readByte() {
  if (readPos_ >= buffer_.size())
    throw std::runtime_error("Byte read out of bounds");
  return buffer_[readPos_++];
}

std::string BufferUtil::readString() {
  int length = readVarInt();
  if (readPos_ + length > buffer_.size())
    throw std::runtime_error("String read out of bounds");
  std::string str(buffer_.begin() + readPos_,
                  buffer_.begin() + readPos_ + length);
  readPos_ += length;
  return str;
}

std::vector<uint8_t> BufferUtil::readByteArray() {
  int length = readVarInt();
  return readBytes(length);
}

std::vector<uint8_t> BufferUtil::readBytes(size_t length) {
  if (readPos_ + length > buffer_.size())
    throw std::runtime_error("ByteArray read out of bounds");
  std::vector<uint8_t> out(buffer_.begin() + readPos_,
                           buffer_.begin() + readPos_ + length);
  readPos_ += length;
  return out;
}

size_t BufferUtil::remaining() const { return buffer_.size() - readPos_; }

const std::vector<uint8_t> &BufferUtil::data() const { return buffer_; }

std::array<uint8_t, 16> parseDashlessUUID(const std::string &hexUUID) {
  mc::log(mc::LogLevel::DEBUG, "Parsing dashless UUID: " + hexUUID);
  if (hexUUID.size() != 32) {
    mc::log(mc::LogLevel::ERROR,
            "UUID length invalid: " + std::to_string(hexUUID.size()));
    throw std::invalid_argument(
        "UUID must be exactly 32 hexadecimal characters.");
  }
  for (char c : hexUUID) {
    if (!isxdigit(c)) {
      mc::log(mc::LogLevel::ERROR,
              "Invalid character in UUID: " + std::string(1, c));
      throw std::invalid_argument("UUID contains non-hexadecimal characters.");
    }
  }
  std::array<uint8_t, 16> uuidBytes;
  try {
    for (size_t i = 0; i < 16; ++i) {
      std::string byteStr = hexUUID.substr(i * 2, 2);
      uuidBytes[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
    }
  } catch (const std::exception &e) {
    mc::log(mc::LogLevel::ERROR,
            "Failed to parse UUID bytes: " + std::string(e.what()));
    throw;
  }
  mc::log(mc::LogLevel::DEBUG, "UUID parsed successfully");
  return uuidBytes;
}

std::string formatUUIDFromBytes(const std::vector<uint8_t> &uuidBytes) {
  if (uuidBytes.size() != 16) {
    throw std::invalid_argument("UUID must be exactly 16 bytes");
  }

  std::stringstream ss;
  ss << std::hex << std::setfill('0');

  for (size_t i = 0; i < 16; ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      ss << '-';
    }
    ss << std::setw(2) << static_cast<unsigned>(uuidBytes[i]);
  }

  return ss.str();
}

} // namespace mc
