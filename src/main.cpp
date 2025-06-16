#include "network/socket.hpp"
#include "protocol/handshake.hpp"
#include "protocol/login_start.hpp"
#include "protocol/encryption_request.hpp"
#include "protocol/login_disconnect.hpp"
#include "util/buffer_utils.hpp"
#include "util/logger.hpp"

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

#include <iostream>
#include "authenticate/auth_device_code.hpp"
#include "authenticate/auth_xbl.hpp"
#include "authenticate/auth_xsts.hpp"
#include "authenticate/auth_minecraft.hpp"
#include "authenticate/token_cache.hpp"
#include "util/logger.hpp"

int main() {
    const std::string clientId = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";

    const std::string msTokenFile = "ms_access_token.txt";
    const std::string xblTokenFile = "xbl_token.txt";
    const std::string xstsTokenFile = "xsts_token.txt";
    const std::string mcTokenFile = "mc_access_token.txt";
    const std::string userhashFile = "userhash.txt";

    try {
        // Microsoft token
        auto msTokenOpt = loadToken(msTokenFile);
        std::string msToken;
        if (!msTokenOpt) {
            auto deviceCodeResp = requestDeviceCode(clientId);
            mc::log(mc::LogLevel::DEBUG, 
                "Visit " + deviceCodeResp.verification_uri + " and enter code: " + deviceCodeResp.user_code);

            msToken = pollToken(clientId, deviceCodeResp.device_code, deviceCodeResp.interval);
            saveToken(msTokenFile, msToken);
            mc::log(mc::LogLevel::INFO, "Microsoft access token saved.");
        } else {
            msToken = *msTokenOpt;
            mc::log(mc::LogLevel::INFO, "Loaded Microsoft access token from cache.");
        }

        // XBL token + userhash
        auto xblTokenOpt = loadToken(xblTokenFile);
        auto userhashOpt = loadToken(userhashFile);
        std::string xblToken, userhash;

        if (!xblTokenOpt || !userhashOpt) {
            auto xblResp = authenticateWithXBL(msToken);
            xblToken = xblResp.token;
            userhash = xblResp.userhash;
            saveToken(xblTokenFile, xblToken);
            saveToken(userhashFile, userhash);
            mc::log(mc::LogLevel::INFO, "XBL token and userhash saved.");
        } else {
            xblToken = *xblTokenOpt;
            userhash = *userhashOpt;
            mc::log(mc::LogLevel::INFO, "Loaded XBL token and userhash from cache.");
        }

        // XSTS token
        auto xstsTokenOpt = loadToken(xstsTokenFile);
        std::string xstsToken;
        if (!xstsTokenOpt) {
            auto xstsResp = getXSTSToken(xblToken);
            xstsToken = xstsResp.token;
            saveToken(xstsTokenFile, xstsToken);
            mc::log(mc::LogLevel::INFO, "XSTS token saved.");
        } else {
            xstsToken = *xstsTokenOpt;
            mc::log(mc::LogLevel::INFO, "Loaded XSTS token from cache.");
        }

        // Minecraft access token
        auto mcTokenOpt = loadToken(mcTokenFile);
        std::string mcToken;
        if (!mcTokenOpt) {
            auto mcResp = loginWithMinecraft(userhash, xstsToken);
            mcToken = mcResp.access_token;
            saveToken(mcTokenFile, mcToken);
            mc::log(mc::LogLevel::INFO, "Minecraft access token saved.");
        } else {
            mcToken = *mcTokenOpt;
            mc::log(mc::LogLevel::INFO, "Loaded Minecraft access token from cache.");
        }

        // Check ownership
        bool owns = checkMinecraftOwnership(mcToken);
        mc::log(mc::LogLevel::INFO, std::string("Minecraft ownership: ") + (owns ? "Yes" : "No"));

    } catch (const std::exception& e) {
        mc::log(mc::LogLevel::ERROR, std::string("Error: ") + e.what());
    }

    return 0;
}
