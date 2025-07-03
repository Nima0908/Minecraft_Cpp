#pragma once

#include "../crypto/aes_cipher.hpp"
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace mc::network {

class Socket {
public:
  Socket();
  ~Socket();

  void connect(const std::string &host, int port);
  void enableEncryption(std::shared_ptr<mc::crypto::AESCipher> cipher);

  void sendPacket(const std::vector<uint8_t> &rawData);
  std::vector<uint8_t> recvPacket();

  int32_t recvVarInt();
  std::string recvString();
  std::vector<uint8_t> recvByteArray();
  void setCompressionThreshold(int threshold);
  uint8_t recvByte();
  std::vector<uint8_t> recv(std::size_t length);

private:
  int compressionThreshold_;
  boost::asio::io_context io_context_;
  boost::asio::ip::tcp::socket socket_;
  bool encryptionEnabled_;
  std::shared_ptr<mc::crypto::AESCipher> cipher_;
  std::deque<uint8_t> decryptedBuffer_;

  std::vector<uint8_t> compressIfNeeded(const std::vector<uint8_t> &data);
  std::vector<uint8_t> decompressIfNeeded(const std::vector<uint8_t> &data);
};

} // namespace mc::network
