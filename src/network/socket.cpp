#include "socket.hpp"
#include "../crypto/aes_cipher.hpp"
#include "../util/buffer_utils.hpp"
#include <iostream>
#include <stdexcept>
#include <deque>  // ensure this is included

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
    encryptionEnabled_ = true;
    std::cout << "[DEBUG] Encryption enabled\n";
}

// Buffered send
void Socket::send(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> output = (encryptionEnabled_ && cipher_) ? cipher_->encrypt(data) : data;
    boost::asio::write(socket_, boost::asio::buffer(output));
}

// Buffered + decrypted read into internal buffer
uint8_t Socket::recvByte() {
    while (decryptedBuffer_.empty()) {
        std::array<uint8_t, 512> encrypted;
        boost::system::error_code ec;
        std::size_t len = socket_.read_some(boost::asio::buffer(encrypted), ec);

        if (ec) {
            throw std::runtime_error("read: " + ec.message());
        }

        std::vector<uint8_t> encryptedVec(encrypted.begin(), encrypted.begin() + len);
        std::vector<uint8_t> decrypted = encryptionEnabled_ && cipher_
            ? cipher_->decrypt(encryptedVec)
            : encryptedVec;

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
