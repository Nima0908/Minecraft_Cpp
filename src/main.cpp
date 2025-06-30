#include "authenticate/auth_device_code.hpp"
#include "authenticate/auth_minecraft.hpp"
#include "authenticate/auth_session.hpp"
#include "authenticate/auth_xbl.hpp"
#include "authenticate/auth_xsts.hpp"
#include "authenticate/token_cache.hpp"
#include "crypto/aes_cipher.hpp"
#include "crypto/encryption.hpp"
#include "network/socket.hpp"
#include "protocol/client/encryption_request.hpp"
#include "protocol/client/login_compression.hpp"
#include "protocol/client/login_disconnect.hpp"
#include "protocol/client/login_finished.hpp"
#include "protocol/server/encryption_response.hpp"
#include "protocol/server/handshake.hpp"
#include "protocol/server/login_acknowledged.hpp"
#include "protocol/server/login_start.hpp"
#include "util/buffer_util.hpp"
#include "util/logger.hpp"

#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";
constexpr int DEFAULT_PORT = 25565;
constexpr int PROTOCOL_VERSION = 770;
constexpr int LOGIN_STATE = 2;

enum class LoginPacketId : int {
  DISCONNECT = 0x00,
  ENCRYPTION_REQUEST = 0x01,
  LOGIN_SUCCESS = 0x02,
  LOGIN_COMPRESSION = 0x03
};

struct AuthTokens {
  std::string msToken;
  std::string xblToken;
  std::string xstsToken;
  std::string mcToken;
  std::string userhash;
  std::string minecraftUUID;
};

struct ServerConnection {
  std::string address;
  std::string username;
  int port = DEFAULT_PORT;
};

std::string getOrCreateToken(const std::string &filename,
                             std::function<std::string()> createToken) {
  auto existingToken = loadToken(filename);
  if (existingToken) {
    return *existingToken;
  }

  std::string newToken = createToken();
  saveToken(filename, newToken);
  return newToken;
}

AuthTokens performAuthentication() {
  AuthTokens tokens;

  tokens.msToken = getOrCreateToken("ms_access_token.txt", []() {
    auto deviceCodeResp = requestDeviceCode(CLIENT_ID);
    mc::log(mc::LogLevel::DEBUG, "Got device code response");
    mc::log(mc::LogLevel::DEBUG,
            "Verification URI: " + deviceCodeResp.verification_uri);
    mc::log(mc::LogLevel::DEBUG, "User Code: " + deviceCodeResp.user_code);
    return pollToken(CLIENT_ID, deviceCodeResp.device_code,
                     deviceCodeResp.interval);
  });

  auto xblTokenOpt = loadToken("xbl_token.txt");
  auto userhashOpt = loadToken("userhash.txt");

  if (!xblTokenOpt || !userhashOpt) {
    auto xblResp = authenticateWithXBL(tokens.msToken);
    tokens.xblToken = xblResp.token;
    tokens.userhash = xblResp.userhash;
    saveToken("xbl_token.txt", tokens.xblToken);
    saveToken("userhash.txt", tokens.userhash);
  } else {
    tokens.xblToken = *xblTokenOpt;
    tokens.userhash = *userhashOpt;
  }

  tokens.xstsToken = getOrCreateToken("xsts_token.txt", [&tokens]() {
    auto xstsResp = getXSTSToken(tokens.xblToken);
    return xstsResp.token;
  });

  tokens.mcToken = getOrCreateToken("mc_access_token.txt", [&tokens]() {
    auto mcResp = loginWithMinecraft(tokens.userhash, tokens.xstsToken);
    return mcResp.access_token;
  });

  tokens.minecraftUUID = getMinecraftUUID(tokens.mcToken);
  mc::log(mc::LogLevel::INFO,
          "Fetched Minecraft UUID: " + tokens.minecraftUUID);

  return tokens;
}

ServerConnection getServerDetails() {
  ServerConnection connection;

  std::cout << "Enter the server address: ";
  std::cin >> connection.address;
  std::cout << "Enter your username: ";
  std::cin >> connection.username;

  return connection;
}

void sendHandshakeAndLogin(mc::Socket &socket,
                           const ServerConnection &connection,
                           const std::string &minecraftUUID) {
  mc::HandshakePacket handshake(PROTOCOL_VERSION, connection.address,
                                connection.port, LOGIN_STATE);
  socket.sendPacket(handshake.serialize());
  mc::log(mc::LogLevel::DEBUG, "Sent handshake packet");

  std::array<uint8_t, 16> uuidBytes = mc::parseDashlessUUID(minecraftUUID);
  mc::LoginStart loginStart(connection.username, uuidBytes);
  socket.sendPacket(loginStart.serialize());
  mc::log(mc::LogLevel::DEBUG,
          "Sent login start packet with username: " + connection.username);
}

void handleEncryptionRequest(mc::Socket &socket, mc::BufferUtil &reader,
                             const std::string &mcToken,
                             const std::string &minecraftUUID) {
  mc::EncryptionRequest encryptionRequest;
  encryptionRequest.serverID = reader.readString();
  encryptionRequest.publicKey = reader.readByteArray();
  encryptionRequest.verifyToken = reader.readByteArray();
  mc::log(mc::LogLevel::DEBUG, "Parsed encryption request");

  auto sharedSecret = mc::encryption::generateSharedSecret();
  auto encryptedSecret =
      mc::encryption::rsaEncrypt(sharedSecret, encryptionRequest.publicKey);
  auto encryptedToken = mc::encryption::rsaEncrypt(
      encryptionRequest.verifyToken, encryptionRequest.publicKey);
  std::string serverHash = mc::encryption::computeServerHash(
      encryptionRequest.serverID, sharedSecret, encryptionRequest.publicKey);

  sendJoinServerRequest(mcToken, minecraftUUID, serverHash);
  mc::EncryptionResponse response(encryptedSecret, encryptedToken);
  socket.sendPacket(response.serialize());

  socket.enableEncryption(std::make_shared<mc::AESCipher>(sharedSecret));
  mc::log(mc::LogLevel::INFO, "AES encryption activated");
}

void handleLoginSuccess(mc::BufferUtil &reader) {
  mc::LoginFinished loginSuccess;
  loginSuccess.read(reader);

  mc::log(mc::LogLevel::DEBUG, "Username: " + loginSuccess.username);

  std::string uuidStr = mc::formatUUIDFromBytes(
      std::vector<uint8_t>(loginSuccess.uuid.begin(), loginSuccess.uuid.end()));
  mc::log(mc::LogLevel::DEBUG, "UUID: " + uuidStr);

  for (const auto &prop : loginSuccess.properties) {
    std::string propLog = "Property: " + prop.name + " = " + prop.value;
    if (prop.signature.has_value()) {
      propLog += " (Signature: " + *prop.signature + ")";
    }
    mc::log(mc::LogLevel::DEBUG, propLog);
  }
}

void handleLoginCompression(mc::Socket &socket, mc::BufferUtil &reader) {
  mc::LoginCompression compressionPacket;
  compressionPacket.threshold = reader.readVarInt();
  mc::log(mc::LogLevel::INFO, "Login compression threshold: " +
                                  std::to_string(compressionPacket.threshold));

  socket.setCompressionThreshold(compressionPacket.threshold);
  mc::log(mc::LogLevel::INFO, "Compression activated with threshold: " +
                                  std::to_string(compressionPacket.threshold));
}

bool processLoginPacket(mc::Socket &socket, const std::string &mcToken,
                        const std::string &minecraftUUID) {
  auto loginPacket = socket.recvPacket();
  mc::BufferUtil reader(loginPacket);
  int packetId = reader.readVarInt();
  mc::log(mc::LogLevel::DEBUG,
          "Received packet ID: " + std::to_string(packetId));

  switch (static_cast<LoginPacketId>(packetId)) {
  case LoginPacketId::DISCONNECT: {
    mc::LoginDisconnect disconnect;
    disconnect.reason = reader.readString();
    mc::log(mc::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
    return false;
  }

  case LoginPacketId::ENCRYPTION_REQUEST:
    handleEncryptionRequest(socket, reader, mcToken, minecraftUUID);
    return true;

  case LoginPacketId::LOGIN_SUCCESS:
    handleLoginSuccess(reader);
    return true;

  case LoginPacketId::LOGIN_COMPRESSION:
    handleLoginCompression(socket, reader);
    return true;

  default:
    mc::log(mc::LogLevel::ERROR,
            "Unexpected packet ID: " + std::to_string(packetId));
    return false;
  }
}

bool processPostEncryptionPackets(mc::Socket &socket) {
  auto nextPacket = socket.recvPacket();
  mc::BufferUtil nextReader(nextPacket);
  int nextPacketId = nextReader.readVarInt();
  mc::log(mc::LogLevel::DEBUG,
          "Received packet ID: " + std::to_string(nextPacketId));

  switch (static_cast<LoginPacketId>(nextPacketId)) {
  case LoginPacketId::LOGIN_COMPRESSION: {
    handleLoginCompression(socket, nextReader);

    auto loginSuccessPacket = socket.recvPacket();
    mc::BufferUtil successReader(loginSuccessPacket);
    int successPacketId = successReader.readVarInt();
    mc::log(mc::LogLevel::DEBUG,
            "Received packet ID: " + std::to_string(successPacketId));

    switch (static_cast<LoginPacketId>(successPacketId)) {
    case LoginPacketId::LOGIN_SUCCESS:
      handleLoginSuccess(successReader);
      return true;

    case LoginPacketId::DISCONNECT: {
      mc::LoginDisconnect disconnect;
      disconnect.reason = successReader.readString();
      mc::log(mc::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
      return false;
    }

    default:
      mc::log(mc::LogLevel::ERROR, "Expected Login Success (0x02), got: " +
                                       std::to_string(successPacketId));
      return false;
    }
  }

  case LoginPacketId::LOGIN_SUCCESS:
    handleLoginSuccess(nextReader);
    return true;

  case LoginPacketId::DISCONNECT: {
    mc::LoginDisconnect disconnect;
    disconnect.reason = nextReader.readString();
    mc::log(mc::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
    return false;
  }

  default:
    mc::log(mc::LogLevel::ERROR,
            "Expected Login Compression (0x03) or Login Success (0x02), got: " +
                std::to_string(nextPacketId));
    return false;
  }
}

int main() {
  try {
    AuthTokens tokens = performAuthentication();

    ServerConnection connection = getServerDetails();

    mc::Socket socket;
    socket.connect(connection.address, connection.port);
    mc::log(mc::LogLevel::DEBUG, "Connected to server " + connection.address);

    sendHandshakeAndLogin(socket, connection, tokens.minecraftUUID);

    if (!processLoginPacket(socket, tokens.mcToken, tokens.minecraftUUID)) {
      return 1;
    }

    if (!processPostEncryptionPackets(socket)) {
      return 1;
    }

    mc::LoginAcknowledged loginAck;
    socket.sendPacket(loginAck.serialize());
    mc::log(mc::LogLevel::DEBUG, "Sent login acknowledgment");

    mc::log(mc::LogLevel::INFO, "Successfully logged into the server!");
    return 0;

  } catch (const std::exception &e) {
    mc::log(mc::LogLevel::ERROR, e.what());
    return 1;
  }
}
