#include "network/socket.hpp"
#include "protocol/server/handshake.hpp"
#include "protocol/server/login_start.hpp"
#include "protocol/client/encryption_request.hpp"
#include "protocol/server/encryption_response.hpp"
#include "protocol/client/login_disconnect.hpp"
#include "util/buffer_utils.hpp"
#include "util/logger.hpp"
#include "util/uuid_utils.hpp"
#include "authenticate/auth_session.hpp"
#include "crypto/encryption.hpp"
#include "crypto/aes_cipher.hpp"
#include "authenticate/auth_device_code.hpp"
#include "authenticate/auth_xbl.hpp"
#include "authenticate/auth_xsts.hpp"
#include "authenticate/auth_minecraft.hpp"
#include "authenticate/token_cache.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

int main() {
    const std::string clientId = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";
    const std::string msTokenFile = "ms_access_token.txt";
    const std::string xblTokenFile = "xbl_token.txt";
    const std::string xstsTokenFile = "xsts_token.txt";
    const std::string mcTokenFile = "mc_access_token.txt";
    const std::string userhashFile = "userhash.txt";

    std::string mcToken;
    std::string minecraftUUID;

    try {
        auto msTokenOpt = loadToken(msTokenFile);
        std::string msToken;
        if (!msTokenOpt) {
            auto deviceCodeResp = requestDeviceCode(clientId);
            mc::log(mc::LogLevel::DEBUG, "Got device code response");
            mc::log(mc::LogLevel::DEBUG, "Verification URI: " + deviceCodeResp.verification_uri);
            mc::log(mc::LogLevel::DEBUG, "User Code: " + deviceCodeResp.user_code);
            msToken = pollToken(clientId, deviceCodeResp.device_code, deviceCodeResp.interval);
            saveToken(msTokenFile, msToken);
            mc::log(mc::LogLevel::DEBUG, "MS Token saved");
        } else {
            msToken = *msTokenOpt;
            mc::log(mc::LogLevel::DEBUG, "Loaded MS Token from cache");
        }

        auto xblTokenOpt = loadToken(xblTokenFile);
        auto userhashOpt = loadToken(userhashFile);
        std::string xblToken, userhash;

        if (!xblTokenOpt || !userhashOpt) {
            auto xblResp = authenticateWithXBL(msToken);
            xblToken = xblResp.token;
            userhash = xblResp.userhash;
            mc::log(mc::LogLevel::DEBUG, "Authenticated with XBL");
            saveToken(xblTokenFile, xblToken);
            saveToken(userhashFile, userhash);
        } else {
            xblToken = *xblTokenOpt;
            userhash = *userhashOpt;
            mc::log(mc::LogLevel::DEBUG, "Loaded XBL token and userhash from cache");
        }

        auto xstsTokenOpt = loadToken(xstsTokenFile);
        std::string xstsToken;
        if (!xstsTokenOpt) {
            auto xstsResp = getXSTSToken(xblToken);
            xstsToken = xstsResp.token;
            saveToken(xstsTokenFile, xstsToken);
            mc::log(mc::LogLevel::DEBUG, "Fetched and saved XSTS Token");
        } else {
            xstsToken = *xstsTokenOpt;
            mc::log(mc::LogLevel::DEBUG, "Loaded XSTS Token from cache");
        }

        auto mcTokenOpt = loadToken(mcTokenFile);
        if (!mcTokenOpt) {
            auto mcResp = loginWithMinecraft(userhash, xstsToken);
            mcToken = mcResp.access_token;
            saveToken(mcTokenFile, mcToken);
            mc::log(mc::LogLevel::DEBUG, "Fetched and saved Minecraft access token");
        } else {
            mcToken = *mcTokenOpt;
            mc::log(mc::LogLevel::DEBUG, "Loaded Minecraft access token from cache");
        }

        minecraftUUID = getMinecraftUUID(mcToken);
        mc::log(mc::LogLevel::INFO, "Fetched Minecraft UUID: " + minecraftUUID);

    } catch (const std::exception& e) {
        mc::log(mc::LogLevel::ERROR, e.what());
        return 1;
    }

    mc::Socket socket;
    std::string address, username;
    const int port = 25565;

    std::cout << "Enter the server address: ";
    std::cin >> address;
    std::cout << "Enter your username: ";
    std::cin >> username;

    try {
        socket.connect(address, port);
        mc::log(mc::LogLevel::DEBUG, "Connected to server " + address);

        mc::HandshakePacket handshake(770, address, port, 2);
        socket.send(handshake.serialize());
        mc::log(mc::LogLevel::DEBUG, "Sent handshake packet");
        
        std::array<uint8_t, 16> uuidBytes = mc::parseDashlessUUID(minecraftUUID);
        mc::LoginStart loginStart(username, uuidBytes);
        socket.send(loginStart.serialize());
        mc::log(mc::LogLevel::DEBUG, "Sent login start packet with username: " + username);

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

        mc::EncryptionRequest encryptionRequest;
        encryptionRequest.read(socket);
        mc::log(mc::LogLevel::DEBUG, "Received encryption request");

        auto sharedSecret = mc::encryption::generateSharedSecret();
        mc::log(mc::LogLevel::DEBUG, "Generated shared secret");

        auto encryptedSecret = mc::encryption::rsaEncrypt(sharedSecret, encryptionRequest.publicKey);
        mc::log(mc::LogLevel::DEBUG, "Encrypted shared secret");

        auto encryptedToken = mc::encryption::rsaEncrypt(encryptionRequest.verifyToken, encryptionRequest.publicKey);
        mc::log(mc::LogLevel::DEBUG, "Encrypted verify token");

        std::string serverHash = mc::encryption::computeServerHash(encryptionRequest.serverID, sharedSecret, encryptionRequest.publicKey);
        mc::log(mc::LogLevel::DEBUG, "Computed server hash: " + serverHash);

        sendJoinServerRequest(mcToken, minecraftUUID, serverHash);
        mc::log(mc::LogLevel::DEBUG, "Sent join server request");

        mc::EncryptionResponse response(encryptedSecret, encryptedToken);
        socket.send(response.serialize());
        socket.enableEncryption(std::make_shared<mc::AESCipher>(sharedSecret));
      
        mc::log(mc::LogLevel::INFO, "AES encryption activated");

    } catch (const std::exception& e) {
        mc::log(mc::LogLevel::ERROR, e.what());
        return 1;
    }

    int loginSuccessLength = socket.recvVarInt();
    int loginSuccessId = socket.recvVarInt();
    mc::log(mc::LogLevel::DEBUG, "Received post-encryption packet ID: " + std::to_string(loginSuccessId));

    if (loginSuccessId == 0x00) {
        mc::LoginDisconnect disconnect;
        disconnect.read(socket);
        mc::log(mc::LogLevel::ERROR, "Disconnected after encryption: " + disconnect.reason);
        return 1;
    }

    if (loginSuccessId != 0x02) {
        mc::log(mc::LogLevel::ERROR, "Expected Login Success packet (0x02), got: " + std::to_string(loginSuccessId));
        return 1;
    }

    std::string playerUUID = socket.recvString();
    std::string playerName = socket.recvString();

    mc::log(mc::LogLevel::INFO, "Login successful! UUID: " + playerUUID + ", Name: " + playerName);

    return 0;
}
