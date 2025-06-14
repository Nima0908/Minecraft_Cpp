#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mc {

class AESCipher;

class Socket {
public:
    Socket();
    ~Socket();

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
    std::shared_ptr<AESCipher> cipher_ = nullptr;
};

}
