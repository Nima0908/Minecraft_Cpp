#include "socket.hpp"
#include "../crypto/aes_cipher.hpp"
#include "../util/buffer_utils.hpp"
#include <iostream>
#include <stdexcept>

namespace mc {

Socket::Socket()
    : socket_(io_context_), encryptionEnabled_(false) {}

Socket::~Socket() {
    boost::system::error_code ec;
    socket_.close(ec);
    if (ec) {
        std::cerr << "Warning: socket close failed: " << ec.message() << "\n";
    }
}

void Socket::connect(const std::string& host, int port) {
    using boost::asio::ip::tcp;
    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket_, endpoints);
}

void Socket::enableEncryption(std::shared_ptr<AESCipher> cipher) {
    cipher_ = cipher;
    encryptionEnabled_ = true;  // Add this flag!
    std::cout << "[DEBUG] Encryption enabled\n";
}

void Socket::send(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> output = (encryptionEnabled_ && cipher_) ? cipher_->encrypt(data) : data;
    boost::asio::write(socket_, boost::asio::buffer(output));
}

std::vector<uint8_t> Socket::recv(std::size_t length) {
    std::vector<uint8_t> buffer(length);
    boost::asio::read(socket_, boost::asio::buffer(buffer));
    
    if (encryptionEnabled_ && cipher_) {
        return cipher_->decrypt(buffer);
    }
    return buffer;
}

uint8_t Socket::recvByte() {
    auto bytes = recv(1);
    return bytes[0];
}

int32_t Socket::recvVarInt() {
    int32_t result = 0;
    int numRead = 0;
    uint8_t byte;
    do {
        byte = recvByte();
        result |= (byte & 0x7F) << (7 * numRead);
        numRead++;
        if (numRead > 5) {
            throw std::runtime_error("VarInt is too big");
        }
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

}
