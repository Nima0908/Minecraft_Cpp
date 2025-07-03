#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mc::utils {
class BufferUtil {
private:
  std::vector<uint8_t> buffer_;
  size_t readPos_;

public:
  BufferUtil();
  BufferUtil(std::vector<uint8_t> data);

  void writeVarInt(int32_t value);
  void writeUInt16(uint16_t value);
  void writeString(const std::string &str);
  void writeByteArray(const std::vector<uint8_t> &bytes);
  void writeBytes(const std::vector<uint8_t> &bytes);

  int32_t readVarInt();
  uint16_t readUInt16();
  uint8_t readByte();
  std::string readString();
  std::vector<uint8_t> readByteArray();
  std::vector<uint8_t> readBytes(size_t length);

  size_t remaining() const;
  const std::vector<uint8_t> &data() const;
};
} // namespace mc::utils
