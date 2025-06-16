#include "network/socket.hpp"
#include "protocol/handshake.hpp"
#include "protocol/login_start.hpp"
#include "protocol/encryption_request.hpp"
#include "protocol/login_disconnect.hpp"
#include "util/buffer_utils.hpp"
#include "util/logger.hpp"

#include "authenticate/device_code.hpp"
#include "authenticate/token_poll.hpp"

#include <iostream>
#include <string>
#include <vector>

/* int main() {
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

        mc::HandshakePacket handshake(770, address, port, 2);
        socket.send(handshake.serialize());
        mc::log(mc::LogLevel::DEBUG, "Sent handshake");

        mc::LoginStart loginStart(username);
        socket.send(loginStart.serialize());

        std::vector<uint8_t> loginPacket = loginStart.serialize();
        std::cout << "LoginStart packet (hex): ";
        for (uint8_t byte : loginPacket) {
            printf("%02X ", byte);
        }

        std::cout << std::endl;


        mc::log(mc::LogLevel::DEBUG, "Sent LoginStart with username: " + username);

        int packetLength = socket.recvVarInt();
        int packetId = socket.recvVarInt();
        mc::log(mc::LogLevel::DEBUG, "Received packet ID: " + std::to_string(packetId));

        if (packetId == 0x00) {
          mc::LoginDisconnect disconnect;
          disconnect.read(socket);
          mc::log(mc::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
          return 1;
        }

        if (packetId != 0x01) {
          mc::log(mc::LogLevel::ERROR, "Expected EncryptionRequest (0x01), got: " + std::to_string(packetId));
          return 1;
        }
 

        mc::log(mc::LogLevel::INFO, "Received EncryptionRequest – proceeding with encryption (not yet implemented)");

        mc::EncryptionRequest encryptionRequest;
        encryptionRequest.read(socket);

        mc::log(mc::LogLevel::INFO, "EncryptionRequest received");
        mc::log(mc::LogLevel::DEBUG, "ServerID: " + encryptionRequest.serverID);
        mc::log(mc::LogLevel::DEBUG, "PublicKey size: " + std::to_string(encryptionRequest.publicKey.size()));
        mc::log(mc::LogLevel::DEBUG, "VerifyToken size: " + std::to_string(encryptionRequest.verifyToken.size()));

        // TODO: Implement encryption handshake and session authentication

    } catch (const std::exception& e) {
        mc::log(mc::LogLevel::ERROR, std::string("Exception occurred: ") + e.what());
        return 1;
    }

    return 0;
} */


int main() {
    const std::string CLIENT_ID = ":()";
    try {
        auto codeRes = mc::auth::requestDeviceCode(CLIENT_ID);
        std::cout << "Go to " << codeRes.verification_uri
                  << " and enter code: " << codeRes.user_code << std::endl;

        auto tokenRes = mc::auth::pollForToken(
            CLIENT_ID, codeRes.device_code, codeRes.interval);

        std::cout << "Access Token: " << tokenRes.access_token << "\n";
        // Save refresh_token if needed: tokenRes.refresh_token

        // Now proceed with Xbox/XSTS/Minecraft login and your session join flow...
    } catch (const std::exception& e) {
        std::cerr << "Authentication error: " << e.what() << std::endl;
        return 1;
    }
}

