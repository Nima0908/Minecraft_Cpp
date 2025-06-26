#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <string>
#include <deque>

namespace mc {

class AESCipher; // Forward declaration

class Socket {
public:
    Socket();
    ~Socket();
    
    std::deque<uint8_t> decryptedBuffer_;

    void connect(const std::string& host, int port);
    void enableEncryption(std::shared_ptr<AESCipher> cipher);
    
    void send(const std::vector<uint8_t>& data);
    std::vector<uint8_t> recv(std::size_t length);
    
    uint8_t recvByte();
    int32_t recvVarInt();
    std::string recvString();
    std::vector<uint8_t> recvByteArray();

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
    std::shared_ptr<AESCipher> cipher_;
    bool encryptionEnabled_; // Add this member variable
};

}
