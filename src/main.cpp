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
#include "util/uuid_utils.hpp"

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
  auto existingToken = mc::auth::loadToken(filename);
  if (existingToken) {
    return *existingToken;
  }

  std::string newToken = createToken();
  mc::auth::saveToken(filename, newToken);
  return newToken;
}

AuthTokens performAuthentication() {
  AuthTokens tokens;

  tokens.msToken = getOrCreateToken("ms_access_token.txt", []() {
    auto deviceCodeResp = mc::auth::requestDeviceCode(CLIENT_ID);
    mc::utils::log(mc::utils::LogLevel::DEBUG, "Got device code response");
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Verification URI: " + deviceCodeResp.verification_uri);
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "User Code: " + deviceCodeResp.user_code);
    return mc::auth::pollToken(CLIENT_ID, deviceCodeResp.device_code,
                               deviceCodeResp.interval);
  });

  auto xblTokenOpt = mc::auth::loadToken("xbl_token.txt");
  auto userhashOpt = mc::auth::loadToken("userhash.txt");

  if (!xblTokenOpt || !userhashOpt) {
    auto xblResp = mc::auth::authenticateWithXBL(tokens.msToken);
    tokens.xblToken = xblResp.token;
    tokens.userhash = xblResp.userhash;
    mc::auth::saveToken("xbl_token.txt", tokens.xblToken);
    mc::auth::saveToken("userhash.txt", tokens.userhash);
  } else {
    tokens.xblToken = *xblTokenOpt;
    tokens.userhash = *userhashOpt;
  }

  tokens.xstsToken = getOrCreateToken("xsts_token.txt", [&tokens]() {
    auto xstsResp = mc::auth::getXSTSToken(tokens.xblToken);
    return xstsResp.token;
  });

  tokens.mcToken = getOrCreateToken("mc_access_token.txt", [&tokens]() {
    auto mcResp =
        mc::auth::loginWithMinecraft(tokens.userhash, tokens.xstsToken);
    return mcResp.access_token;
  });

  tokens.minecraftUUID = mc::auth::getMinecraftUUID(tokens.mcToken);
  mc::utils::log(mc::utils::LogLevel::INFO,
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

void sendHandshakeAndLogin(mc::network::Socket &socket,
                           const ServerConnection &connection,
                           const std::string &minecraftUUID) {
  mc::protocol::server::HandshakePacket handshake(
      PROTOCOL_VERSION, connection.address, connection.port, LOGIN_STATE);
  socket.sendPacket(handshake.serialize());
  mc::utils::log(mc::utils::LogLevel::DEBUG, "Sent handshake packet");

  std::array<uint8_t, 16> uuidBytes =
      mc::utils::parseDashlessUUID(minecraftUUID);
  mc::protocol::server::LoginStart loginStart(connection.username, uuidBytes);
  socket.sendPacket(loginStart.serialize());
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Sent login start packet with username: " +
                     connection.username);
}

void handleEncryptionRequest(mc::network::Socket &socket,
                             mc::utils::BufferUtil &reader,
                             const std::string &mcToken,
                             const std::string &minecraftUUID) {
  mc::protocol::client::EncryptionRequest encryptionRequest;
  encryptionRequest.serverID = reader.readString();
  encryptionRequest.publicKey = reader.readByteArray();
  encryptionRequest.verifyToken = reader.readByteArray();
  mc::utils::log(mc::utils::LogLevel::DEBUG, "Parsed encryption request");

  auto sharedSecret = mc::crypto::generateSharedSecret();
  auto encryptedSecret =
      mc::crypto::rsaEncrypt(sharedSecret, encryptionRequest.publicKey);
  auto encryptedToken = mc::crypto::rsaEncrypt(encryptionRequest.verifyToken,
                                               encryptionRequest.publicKey);
  std::string serverHash = mc::crypto::computeServerHash(
      encryptionRequest.serverID, sharedSecret, encryptionRequest.publicKey);

  mc::auth::sendJoinServerRequest(mcToken, minecraftUUID, serverHash);
  mc::protocol::server::EncryptionResponse response(encryptedSecret,
                                                    encryptedToken);
  socket.sendPacket(response.serialize());

  socket.enableEncryption(
      std::make_shared<mc::crypto::AESCipher>(sharedSecret));
  mc::utils::log(mc::utils::LogLevel::INFO, "AES encryption activated");
}

void handleLoginSuccess(mc::utils::BufferUtil &reader) {
  mc::protocol::client::LoginFinished loginSuccess;
  loginSuccess.read(reader);

  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Username: " + loginSuccess.username);

  std::string uuidStr = mc::utils::formatUUIDFromBytes(
      std::vector<uint8_t>(loginSuccess.uuid.begin(), loginSuccess.uuid.end()));
  mc::utils::log(mc::utils::LogLevel::DEBUG, "UUID: " + uuidStr);

  for (const auto &prop : loginSuccess.properties) {
    std::string propLog = "Property: " + prop.name + " = " + prop.value;
    if (prop.signature.has_value()) {
      propLog += " (Signature: " + *prop.signature + ")";
    }
    mc::utils::log(mc::utils::LogLevel::DEBUG, propLog);
  }
}

void handleLoginCompression(mc::network::Socket &socket,
                            mc::utils::BufferUtil &reader) {
  mc::protocol::client::LoginCompression compressionPacket;
  compressionPacket.threshold = reader.readVarInt();
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Login compression threshold: " +
                     std::to_string(compressionPacket.threshold));

  socket.setCompressionThreshold(compressionPacket.threshold);
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Compression activated with threshold: " +
                     std::to_string(compressionPacket.threshold));
}

bool processLoginPacket(mc::network::Socket &socket, const std::string &mcToken,
                        const std::string &minecraftUUID) {
  auto loginPacket = socket.recvPacket();
  mc::utils::BufferUtil reader(loginPacket);
  int packetId = reader.readVarInt();
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Received packet ID: " + std::to_string(packetId));

  switch (static_cast<LoginPacketId>(packetId)) {
  case LoginPacketId::DISCONNECT: {
    mc::protocol::client::LoginDisconnect disconnect;
    disconnect.reason = reader.readString();
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Disconnected: " + disconnect.reason);
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
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Unexpected packet ID: " + std::to_string(packetId));
    return false;
  }
}

bool processPostEncryptionPackets(mc::network::Socket &socket) {
  auto nextPacket = socket.recvPacket();
  mc::utils::BufferUtil nextReader(nextPacket);
  int nextPacketId = nextReader.readVarInt();
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Received packet ID: " + std::to_string(nextPacketId));

  switch (static_cast<LoginPacketId>(nextPacketId)) {
  case LoginPacketId::LOGIN_COMPRESSION: {
    handleLoginCompression(socket, nextReader);

    auto loginSuccessPacket = socket.recvPacket();
    mc::utils::BufferUtil successReader(loginSuccessPacket);
    int successPacketId = successReader.readVarInt();
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Received packet ID: " + std::to_string(successPacketId));

    switch (static_cast<LoginPacketId>(successPacketId)) {
    case LoginPacketId::LOGIN_SUCCESS:
      handleLoginSuccess(successReader);
      return true;

    case LoginPacketId::DISCONNECT: {
      mc::protocol::client::LoginDisconnect disconnect;
      disconnect.reason = successReader.readString();
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Disconnected: " + disconnect.reason);
      return false;
    }

    default:
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Expected Login Success (0x02), got: " +
                         std::to_string(successPacketId));
      return false;
    }
  }

  case LoginPacketId::LOGIN_SUCCESS:
    handleLoginSuccess(nextReader);
    return true;

  case LoginPacketId::DISCONNECT: {
    mc::protocol::client::LoginDisconnect disconnect;
    disconnect.reason = nextReader.readString();
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Disconnected: " + disconnect.reason);
    return false;
  }

  default:
    mc::utils::log(
        mc::utils::LogLevel::ERROR,
        "Expected Login Compression (0x03) or Login Success (0x02), got: " +
            std::to_string(nextPacketId));
    return false;
  }
}

int main() {
  try {
    AuthTokens tokens = performAuthentication();

    ServerConnection connection = getServerDetails();

    mc::network::Socket socket;
    socket.connect(connection.address, connection.port);
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Connected to server " + connection.address);

    sendHandshakeAndLogin(socket, connection, tokens.minecraftUUID);

    if (!processLoginPacket(socket, tokens.mcToken, tokens.minecraftUUID)) {
      return 1;
    }

    if (!processPostEncryptionPackets(socket)) {
      return 1;
    }

    mc::protocol::server::LoginAcknowledged loginAck;
    socket.sendPacket(loginAck.serialize());
    mc::utils::log(mc::utils::LogLevel::DEBUG, "Sent login acknowledgment");

    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Successfully logged into the server!");
    return 0;

  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR, e.what());
    return 1;
  }
}
