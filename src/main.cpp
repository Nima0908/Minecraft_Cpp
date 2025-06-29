#include "network/socket.hpp"
#include "protocol/server/handshake.hpp"
#include "protocol/server/login_start.hpp"
#include "protocol/client/encryption_request.hpp"
#include "protocol/server/encryption_response.hpp"
#include "protocol/client/login_disconnect.hpp"
#include "protocol/client/login_compression.hpp"
#include "protocol/client/login_finished.hpp"
#include "util/buffer_util.hpp"
#include "util/logger.hpp"
//#include "util/uuid_utils.hpp"
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
        } else {
            msToken = *msTokenOpt;
        }

        auto xblTokenOpt = loadToken(xblTokenFile);
        auto userhashOpt = loadToken(userhashFile);
        std::string xblToken, userhash;

        if (!xblTokenOpt || !userhashOpt) {
            auto xblResp = authenticateWithXBL(msToken);
            xblToken = xblResp.token;
            userhash = xblResp.userhash;
            saveToken(xblTokenFile, xblToken);
            saveToken(userhashFile, userhash);
        } else {
            xblToken = *xblTokenOpt;
            userhash = *userhashOpt;
        }

        auto xstsTokenOpt = loadToken(xstsTokenFile);
        std::string xstsToken;
        if (!xstsTokenOpt) {
            auto xstsResp = getXSTSToken(xblToken);
            xstsToken = xstsResp.token;
            saveToken(xstsTokenFile, xstsToken);
        } else {
            xstsToken = *xstsTokenOpt;
        }

        auto mcTokenOpt = loadToken(mcTokenFile);
        if (!mcTokenOpt) {
            auto mcResp = loginWithMinecraft(userhash, xstsToken);
            mcToken = mcResp.access_token;
            saveToken(mcTokenFile, mcToken);
        } else {
            mcToken = *mcTokenOpt;
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
        socket.sendPacket(handshake.serialize());
        mc::log(mc::LogLevel::DEBUG, "Sent handshake packet");

        std::array<uint8_t, 16> uuidBytes = mc::parseDashlessUUID(minecraftUUID);
        mc::LoginStart loginStart(username, uuidBytes);
        socket.sendPacket(loginStart.serialize());
        mc::log(mc::LogLevel::DEBUG, "Sent login start packet with username: " + username);

        auto loginPacket = socket.recvPacket();
        mc::BufferUtil reader(loginPacket);
        int packetId = reader.readVarInt();
        mc::log(mc::LogLevel::DEBUG, "Received packet ID: " + std::to_string(packetId));

        if (packetId == 0x00) {
            mc::LoginDisconnect disconnect;
            disconnect.reason = reader.readString();
            mc::log(mc::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
            return 1;
        }

        if (packetId != 0x01) {
            mc::log(mc::LogLevel::ERROR, "Expected EncryptionRequest (0x01), got: " + std::to_string(packetId));
            return 1;
        }

        mc::EncryptionRequest encryptionRequest;
        encryptionRequest.serverID = reader.readString();
        encryptionRequest.publicKey = reader.readByteArray();
        encryptionRequest.verifyToken = reader.readByteArray();
        mc::log(mc::LogLevel::DEBUG, "Parsed encryption request");

        auto sharedSecret = mc::encryption::generateSharedSecret();
        auto encryptedSecret = mc::encryption::rsaEncrypt(sharedSecret, encryptionRequest.publicKey);
        auto encryptedToken = mc::encryption::rsaEncrypt(encryptionRequest.verifyToken, encryptionRequest.publicKey);
        std::string serverHash = mc::encryption::computeServerHash(encryptionRequest.serverID, sharedSecret, encryptionRequest.publicKey);

        sendJoinServerRequest(mcToken, minecraftUUID, serverHash);
        mc::EncryptionResponse response(encryptedSecret, encryptedToken);
        socket.sendPacket(response.serialize());

        socket.enableEncryption(std::make_shared<mc::AESCipher>(sharedSecret));
        mc::log(mc::LogLevel::INFO, "AES encryption activated");
     
        auto nextPacket = socket.recvPacket();
        mc::BufferUtil nextReader(nextPacket);
        int nextPacketId = nextReader.readVarInt();
        mc::log(mc::LogLevel::DEBUG, "Received packet ID: " + std::to_string(nextPacketId));

        if (nextPacketId == 0x03) {
            mc::LoginCompression compressionPacket;
            compressionPacket.threshold = nextReader.readVarInt();
            mc::log(mc::LogLevel::INFO, "Login compression threshold: " + std::to_string(compressionPacket.threshold));
            
            socket.setCompressionThreshold(compressionPacket.threshold);
            mc::log(mc::LogLevel::INFO, "Compression activated with threshold: " + std::to_string(compressionPacket.threshold));

            auto loginSuccessPacket = socket.recvPacket();
            mc::BufferUtil successReader(loginSuccessPacket);
            int successPacketId = successReader.readVarInt();
            mc::log(mc::LogLevel::DEBUG, "Received packet ID: " + std::to_string(successPacketId));

            if (successPacketId == 0x02) {
                mc::LoginFinished loginSuccess;
                std::vector<uint8_t> uuidVector = successReader.readByteArray(); 
                loginSuccess.username = successReader.readString();
                
                int numProperties = successReader.readVarInt();
                mc::log(mc::LogLevel::DEBUG, "Number of properties: " + std::to_string(numProperties));
                
                for (int i = 0; i < numProperties; i++) {
                    std::string name = successReader.readString();
                    std::string value = successReader.readString();
                    bool isSigned = successReader.readByte() != 0;
                    std::string signature = "";
                    if (isSigned) {
                        signature = successReader.readString();
                    }
                    mc::log(mc::LogLevel::DEBUG, "Property: " + name + " = " + value + (isSigned ? " (signed)" : ""));
                }

                std::string uuidStr = mc::formatUUIDFromBytes(uuidVector);
                mc::log(mc::LogLevel::INFO, "Login successful! UUID: " + uuidStr + ", Username: " + loginSuccess.username);
                
            } else if (successPacketId == 0x00) {
                mc::LoginDisconnect disconnect;
                disconnect.reason = successReader.readString();
                mc::log(mc::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
                return 1;
            } else {
                mc::log(mc::LogLevel::ERROR, "Expected Login Success (0x02), got: " + std::to_string(successPacketId));
                return 1;
            }

        } else if (nextPacketId == 0x02) {
            mc::LoginFinished loginSuccess;
            std::vector<uint8_t> uuidVector = nextReader.readByteArray(); 
            loginSuccess.username = nextReader.readString();
            
            int numProperties = nextReader.readVarInt();
            mc::log(mc::LogLevel::DEBUG, "Number of properties: " + std::to_string(numProperties));
            
            for (int i = 0; i < numProperties; i++) {
                std::string name = nextReader.readString();
                std::string value = nextReader.readString();
                bool isSigned = nextReader.readByte() != 0;
                std::string signature = "";
                if (isSigned) {
                    signature = nextReader.readString();
                }
                mc::log(mc::LogLevel::DEBUG, "Property: " + name + " = " + value + (isSigned ? " (signed)" : ""));
            }

            std::string uuidStr = mc::formatUUIDFromBytes(uuidVector);
            mc::log(mc::LogLevel::INFO, "Login successful! UUID: " + uuidStr + ", Username: " + loginSuccess.username);
            
        } else if (nextPacketId == 0x00) {
            mc::LoginDisconnect disconnect;
            disconnect.reason = nextReader.readString();
            mc::log(mc::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
            return 1;
        } else {
            mc::log(mc::LogLevel::ERROR, "Expected Login Compression (0x03) or Login Success (0x02), got: " + std::to_string(nextPacketId));
            return 1;
        }

    } catch (const std::exception& e) {
        mc::log(mc::LogLevel::ERROR, e.what());
        return 1;
    }

    mc::log(mc::LogLevel::INFO, "Successfully logged into the server!");
    return 0;
}
