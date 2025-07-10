#include "socket.hpp"
#include "../buffer/read_buffer.hpp"
#include "../buffer/write_buffer.hpp"
#include "../util/compression_util.hpp"
#include <stdexcept>

namespace mc::network {

Socket::Socket()
    : socket_(io_context_), encryptionEnabled_(false),
      compressionThreshold_(-1) {}

Socket::~Socket() {
  std::lock_guard lock(mutex_);
  boost::system::error_code ec;
  socket_.close(ec);
}

void Socket::connect(const std::string &host, int port) {
  using boost::asio::ip::tcp;
  tcp::resolver resolver(io_context_);
  auto endpoints = resolver.resolve(host, std::to_string(port));
  std::lock_guard lock(mutex_);
  boost::asio::connect(socket_, endpoints);
}

void Socket::enableEncryption(std::shared_ptr<mc::crypto::AESCipher> cipher) {
  std::lock_guard lock(mutex_);
  cipher_ = std::move(cipher);
  encryptionEnabled_ = true;
}

void Socket::setCompressionThreshold(int threshold) {
  std::lock_guard lock(mutex_);
  compressionThreshold_ = threshold;
}

void Socket::sendPacket(const std::vector<uint8_t> &rawData) {
  std::vector<uint8_t> compressed = compressIfNeeded(rawData);
  mc::buffer::WriteBuffer buf;
  buf.writeVarInt(static_cast<int32_t>(compressed.size()));
  buf.writeBytes(compressed);
  std::vector<uint8_t> compiled = buf.compile();

  std::lock_guard lock(mutex_);
  std::vector<uint8_t> output =
      (encryptionEnabled_ && cipher_) ? cipher_->encrypt(compiled) : compiled;

  boost::asio::write(socket_, boost::asio::buffer(output));
}

std::vector<uint8_t> Socket::recvPacket() {
  int32_t length = recvVarInt();
  std::vector<uint8_t> payload = recv(length);
  return decompressIfNeeded(payload);
}

uint8_t Socket::recvByte() {
  std::lock_guard lock(mutex_);
  while (decryptedBuffer_.empty()) {
    fillDecryptedBuffer();
  }
  uint8_t byte = decryptedBuffer_.front();
  decryptedBuffer_.pop_front();
  return byte;
}

std::vector<uint8_t> Socket::recv(std::size_t length) {
  std::vector<uint8_t> output;
  output.reserve(length);
  for (std::size_t i = 0; i < length; ++i) {
    output.push_back(recvByte());
  }
  return output;
}

int32_t Socket::recvVarInt() {
  int32_t result = 0;
  int numRead = 0;
  uint8_t byte;
  do {
    byte = recvByte();
    result |= (byte & 0x7F) << (7 * numRead++);
    if (numRead > 5)
      throw std::runtime_error("VarInt is too big");
  } while (byte & 0x80);
  return result;
}

std::string Socket::recvString() {
  int length = recvVarInt();
  std::vector<uint8_t> bytes = recv(length);
  return std::string(bytes.begin(), bytes.end());
}

std::vector<uint8_t> Socket::recvByteArray() {
  int length = recvVarInt();
  return recv(length);
}

std::vector<uint8_t>
Socket::compressIfNeeded(const std::vector<uint8_t> &data) {
  std::lock_guard lock(mutex_);
  if (compressionThreshold_ < 0)
    return data;

  mc::buffer::WriteBuffer buf;

  if (static_cast<int>(data.size()) >= compressionThreshold_) {
    std::vector<uint8_t> compressed = mc::utils::compress(data);
    buf.writeVarInt(static_cast<int32_t>(data.size()));
    buf.writeBytes(compressed);
  } else {
    buf.writeVarInt(0);
    buf.writeBytes(data);
  }

  return buf.compile();
}

std::vector<uint8_t>
Socket::decompressIfNeeded(const std::vector<uint8_t> &data) {
  std::lock_guard lock(mutex_);
  if (compressionThreshold_ < 0)
    return data;

  mc::buffer::ReadBuffer buf(data);
  int32_t uncompressedLength = buf.readVarInt();

  if (uncompressedLength == 0) {
    return buf.readBytes(buf.remaining());
  }

  std::vector<uint8_t> compressedData = buf.readBytes(buf.remaining());
  std::vector<uint8_t> decompressed = mc::utils::decompress(compressedData);

  if (static_cast<int32_t>(decompressed.size()) != uncompressedLength) {
    throw std::runtime_error("Decompressed size mismatch");
  }

  return decompressed;
}

void Socket::fillDecryptedBuffer() {
  std::array<uint8_t, 512> encrypted;
  boost::system::error_code ec;

  std::size_t len = socket_.read_some(boost::asio::buffer(encrypted), ec);
  if (ec)
    throw std::runtime_error("recvByte failed: " + ec.message());

  std::vector<uint8_t> data(encrypted.begin(), encrypted.begin() + len);
  std::vector<uint8_t> decrypted =
      (encryptionEnabled_ && cipher_) ? cipher_->decrypt(data) : data;

  decryptedBuffer_.insert(decryptedBuffer_.end(), decrypted.begin(),
                          decrypted.end());
}

} // namespace mc::network
