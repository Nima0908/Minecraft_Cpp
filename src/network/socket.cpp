#include "socket.hpp"
#include "../util/buffer_util.hpp"
#include "../util/compression_util.hpp"
#include <stdexcept>
#include <zlib.h>
#include <iostream>

namespace mc {

Socket::Socket()
    : socket_(io_context_), encryptionEnabled_(false), compressionThreshold_(-1) {}

Socket::~Socket() {
    boost::system::error_code ec;
    socket_.close(ec);
}

void Socket::connect(const std::string& host, int port) {
    using boost::asio::ip::tcp;
    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket_, endpoints);
}

void Socket::enableEncryption(std::shared_ptr<AESCipher> cipher) {
    cipher_ = cipher;
    encryptionEnabled_ = true;
}

void Socket::setCompressionThreshold(int threshold) {
    compressionThreshold_ = threshold;
}

// --- Sending ---
void Socket::sendPacket(const std::vector<uint8_t>& rawData) {
    std::vector<uint8_t> compressed = compressIfNeeded(rawData);
    BufferUtil buf;
    buf.writeVarInt(static_cast<int32_t>(compressed.size()));
    buf.writeBytes(compressed);
    
    std::vector<uint8_t> output = (encryptionEnabled_ && cipher_)
        ? cipher_->encrypt(buf.data())
        : buf.data();
    
    boost::asio::write(socket_, boost::asio::buffer(output));
}

// --- Receiving ---
std::vector<uint8_t> Socket::recvPacket() {
    int32_t length = recvVarInt();
    std::vector<uint8_t> payload = recv(length);
    return decompressIfNeeded(payload);
}

uint8_t Socket::recvByte() {
    while (decryptedBuffer_.empty()) {
        std::array<uint8_t, 512> encrypted;
        boost::system::error_code ec;
        std::size_t len = socket_.read_some(boost::asio::buffer(encrypted), ec);
        if (ec) throw std::runtime_error("recvByte failed: " + ec.message());
        
        std::vector<uint8_t> data(encrypted.begin(), encrypted.begin() + len);
        std::vector<uint8_t> decrypted = (encryptionEnabled_ && cipher_)
            ? cipher_->decrypt(data)
            : data;
        
        decryptedBuffer_.insert(decryptedBuffer_.end(), decrypted.begin(), decrypted.end());
    }
    
    uint8_t byte = decryptedBuffer_.front();
    decryptedBuffer_.pop_front();
    return byte;
}

std::vector<uint8_t> Socket::recv(std::size_t length) {
    std::vector<uint8_t> output;
    while (output.size() < length) {
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
        if (numRead > 5) throw std::runtime_error("VarInt is too big");
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

std::vector<uint8_t> Socket::compressIfNeeded(const std::vector<uint8_t>& data) {
    if (compressionThreshold_ < 0) {
        return data;
    }
    
    BufferUtil buf;
    
    if (static_cast<int>(data.size()) >= compressionThreshold_) {
        // Compress the data
        std::vector<uint8_t> compressed = mc::compression::compress(data);
        
        buf.writeVarInt(static_cast<int32_t>(data.size())); 
        buf.writeBytes(compressed);
    } else {
        buf.writeVarInt(0); 
        buf.writeBytes(data);
    }
    
    return buf.data();
}

std::vector<uint8_t> Socket::decompressIfNeeded(const std::vector<uint8_t>& data) {
    if (compressionThreshold_ < 0) {
        return data;
    }
    
    BufferUtil buf(data);
    
    int32_t uncompressedLength = buf.readVarInt();
    
    if (uncompressedLength == 0) {
        return buf.readBytes(buf.remaining());
    } else {
        std::vector<uint8_t> compressedData = buf.readBytes(buf.remaining());
        std::vector<uint8_t> decompressed = mc::compression::decompress(compressedData);
        
        if (static_cast<int32_t>(decompressed.size()) != uncompressedLength) {
            throw std::runtime_error("Decompressed data size mismatch: expected " + 
                                   std::to_string(uncompressedLength) + 
                                   ", got " + std::to_string(decompressed.size()));
        }
        
        return decompressed;
    }
}

} // namespace mc
