#include "network/socket.hpp"
#include "protocol/handshake.hpp"
#include "protocol/login_start.hpp"
#include "util/buffer_utils.hpp"
#include "util/logger.hpp"

#include <iostream>
#include <string>
#include <vector>

int main() {
    mc::Socket socket;

    std::string address;
    std::string username;
    const int port = 25565;

    std::cout << "Enter the server address: ";
    std::cin >> address;
    std::cout << "Enter your username: ";
    std::cin >> username;

    try {
        mc::log(mc::LogLevel::INFO, "Connecting to " + address + ":" + std::to_string(port));
        socket.connect(address, port);
        mc::log(mc::LogLevel::INFO, "Connection successful");

        mc::HandshakePacket handshake(
            760,       // Protocol version
            address,   // Server address
            port,      // Server port
            2          // Next state: login
        );
        auto handshakeData = handshake.serialize();
  
        socket.send(handshakeData);
        mc::log(mc::LogLevel::DEBUG, "Sent handshake");

        mc::LoginStart loginStart(username);
        auto loginData = loginStart.serialize();
        socket.send(loginData);
        mc::log(mc::LogLevel::DEBUG, "Sent LoginStart with username: " + username);

        int packetLength = socket.recvVarInt();
        int packetId = socket.recvVarInt();
        mc::log(mc::LogLevel::DEBUG, "Received packet ID: " + std::to_string(packetId));

        if (packetId != 0x01) {
            mc::log(mc::LogLevel::ERROR, "Expected EncryptionRequest (0x01), got: " + std::to_string(packetId));
            return 1;
        }

        mc::log(mc::LogLevel::INFO, "Received EncryptionRequest – proceeding with encryption (not yet implemented)");

        // TODO: Handle encryption setup here.

    } catch (const std::exception& e) {
        mc::log(mc::LogLevel::ERROR, std::string("Exception occurred: ") + e.what());
        return 1;
    }

    return 0;
}

