#include "main.hpp"
#include "authenticate/auth_device_code.hpp"
#include "authenticate/auth_minecraft.hpp"
#include "authenticate/auth_session.hpp"
#include "authenticate/auth_xbl.hpp"
#include "authenticate/auth_xsts.hpp"
#include "authenticate/token_cache.hpp"
#include "buffer/read_buffer.hpp"
#include "buffer/write_buffer.hpp"
#include "crypto/aes_cipher.hpp"
#include "crypto/encryption.hpp"
#include "handler/packet_handler.hpp"
#include "network/socket.hpp"
#include "protocol/client/handshaking/handshake.hpp"
#include "protocol/client/login/encryption_response.hpp"
#include "protocol/client/login/login_acknowledged.hpp"
#include "protocol/client/login/login_start.hpp"
#include "protocol/server/login/encryption_request.hpp"
#include "protocol/server/login/login_compression.hpp"
#include "protocol/server/login/login_disconnect.hpp"
#include "protocol/server/login/login_finished.hpp"
#include "util/logger.hpp"
#include "util/uuid_utils.hpp"

#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";

using namespace mc;

enum class LoginPacketId : int {
  DISCONNECT = 0x00,
  ENCRYPTION_REQUEST = 0x01,
  LOGIN_SUCCESS = 0x02,
  LOGIN_COMPRESSION = 0x03
};

std::string getOrCreateToken(const std::string &filename,
                             std::function<std::string()> createToken) {
  auto existingToken = auth::loadToken(filename);
  if (existingToken)
    return *existingToken;

  std::string newToken = createToken();
  auth::saveToken(filename, newToken);
  return newToken;
}

AuthTokens performAuthentication() {
  AuthTokens tokens;

  tokens.msToken = getOrCreateToken("ms_access_token.txt", []() {
    auto deviceCodeResp = auth::requestDeviceCode(CLIENT_ID);
    utils::log(utils::LogLevel::DEBUG, "Got device code response");
    utils::log(utils::LogLevel::DEBUG,
               "Verification URI: " + deviceCodeResp.verification_uri);
    utils::log(utils::LogLevel::DEBUG,
               "User Code: " + deviceCodeResp.user_code);
    return auth::pollToken(CLIENT_ID, deviceCodeResp.device_code,
                           deviceCodeResp.interval);
  });

  auto xblTokenOpt = auth::loadToken("xbl_token.txt");
  auto userhashOpt = auth::loadToken("userhash.txt");

  if (!xblTokenOpt || !userhashOpt) {
    auto xblResp = auth::authenticateWithXBL(tokens.msToken);
    tokens.xblToken = xblResp.token;
    tokens.userhash = xblResp.userhash;
    auth::saveToken("xbl_token.txt", tokens.xblToken);
    auth::saveToken("userhash.txt", tokens.userhash);
  } else {
    tokens.xblToken = *xblTokenOpt;
    tokens.userhash = *userhashOpt;
  }

  tokens.xstsToken = getOrCreateToken("xsts_token.txt", [&tokens]() {
    auto xstsResp = auth::getXSTSToken(tokens.xblToken);
    return xstsResp.token;
  });

  tokens.mcToken = getOrCreateToken("mc_access_token.txt", [&tokens]() {
    auto mcResp = auth::loginWithMinecraft(tokens.userhash, tokens.xstsToken);
    return mcResp.access_token;
  });

  tokens.minecraftUUID = auth::getMinecraftUUID(tokens.mcToken);
  utils::log(utils::LogLevel::INFO,
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

void sendHandshakeAndLogin(network::Socket &socket,
                           const ServerConnection &connection,
                           const std::string &minecraftUUID) {
  mc::handler::PacketHandler handler(socket);

  handler.sendPacket<mc::protocol::client::handshaking::HandshakePacket>(
      PROTOCOL_VERSION, connection.address, connection.port, LOGIN_STATE);

  utils::log(utils::LogLevel::DEBUG, "Sent handshake packet");

  std::array<uint8_t, 16> uuidBytes = utils::parseDashlessUUID(minecraftUUID);
  mc::buffer::WriteBuffer write;
  protocol::client::login::LoginStart loginStart(connection.username,
                                                 uuidBytes);
  socket.sendPacket(loginStart.serialize(write));
  utils::log(utils::LogLevel::DEBUG,
             "Sent login start packet with username: " + connection.username);
}

void handleEncryptionRequest(network::Socket &socket, buffer::ReadBuffer &read,
                             const std::string &mcToken,
                             const std::string &minecraftUUID) {
  protocol::server::login::EncryptionRequest encryptionRequest;
  encryptionRequest.read(read);
  utils::log(utils::LogLevel::DEBUG, "Parsed encryption request");

  auto sharedSecret = crypto::generateSharedSecret();
  auto encryptedSecret =
      crypto::rsaEncrypt(sharedSecret, encryptionRequest.publicKey);
  auto encryptedToken = crypto::rsaEncrypt(encryptionRequest.verifyToken,
                                           encryptionRequest.publicKey);
  std::string serverHash = crypto::computeServerHash(
      encryptionRequest.serverID, sharedSecret, encryptionRequest.publicKey);

  auth::sendJoinServerRequest(mcToken, minecraftUUID, serverHash);
  protocol::client::login::EncryptionResponse response(encryptedSecret,
                                                       encryptedToken);

  mc::buffer::WriteBuffer write;
  socket.sendPacket(response.serialize(write));
  socket.enableEncryption(std::make_shared<crypto::AESCipher>(sharedSecret));

  utils::log(utils::LogLevel::INFO, "AES encryption activated");
}

void handleLoginSuccess(buffer::ReadBuffer &reader) {
  protocol::server::login::LoginFinished loginSuccess;
  loginSuccess.read(reader);

  utils::log(utils::LogLevel::DEBUG, "Username: " + loginSuccess.username);
  std::string uuidStr = utils::formatUUIDFromBytes(
      {loginSuccess.uuid.begin(), loginSuccess.uuid.end()});
  utils::log(utils::LogLevel::DEBUG, "UUID: " + uuidStr);

  for (const auto &prop : loginSuccess.properties) {
    std::string propLog = "Property: " + prop.name + " = " + prop.value;
    if (prop.signature.has_value()) {
      propLog += " (Signature: " + *prop.signature + ")";
    }
    utils::log(utils::LogLevel::DEBUG, propLog);
  }
}

void handleLoginCompression(network::Socket &socket, buffer::ReadBuffer &read) {
  protocol::server::login::LoginCompression compressionPacket;
  compressionPacket.read(read);

  utils::log(utils::LogLevel::INFO,
             "Login compression threshold: " +
                 std::to_string(compressionPacket.threshold));
  socket.setCompressionThreshold(compressionPacket.threshold);
}

bool processLoginPacket(network::Socket &socket, const std::string &mcToken,
                        const std::string &minecraftUUID) {
  auto loginPacket = socket.recvPacket();
  buffer::ReadBuffer read(loginPacket);
  int packetId = read.readVarInt();

  utils::log(utils::LogLevel::DEBUG,
             "Received packet ID: " + std::to_string(packetId));

  switch (static_cast<LoginPacketId>(packetId)) {
  case LoginPacketId::DISCONNECT: {
    protocol::server::login::LoginDisconnect disconnect;
    disconnect.read(read);
    utils::log(utils::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
    return false;
  }
  case LoginPacketId::ENCRYPTION_REQUEST:
    handleEncryptionRequest(socket, read, mcToken, minecraftUUID);
    return true;
  case LoginPacketId::LOGIN_SUCCESS:
    handleLoginSuccess(read);
    return true;
  case LoginPacketId::LOGIN_COMPRESSION:
    handleLoginCompression(socket, read);
    return true;
  default:
    utils::log(utils::LogLevel::ERROR,
               "Unexpected packet ID: " + std::to_string(packetId));
    return false;
  }
}

bool processPostEncryptionPackets(network::Socket &socket) {
  auto nextPacket = socket.recvPacket();
  buffer::ReadBuffer read(nextPacket);
  int packetId = read.readVarInt();

  utils::log(utils::LogLevel::DEBUG,
             "Received packet ID: " + std::to_string(packetId));

  switch (static_cast<LoginPacketId>(packetId)) {
  case LoginPacketId::LOGIN_COMPRESSION:
    handleLoginCompression(socket, read);
    return processPostEncryptionPackets(socket);
  case LoginPacketId::LOGIN_SUCCESS:
    handleLoginSuccess(read);
    return true;
  case LoginPacketId::DISCONNECT: {
    protocol::server::login::LoginDisconnect disconnect;
    disconnect.read(read);
    utils::log(utils::LogLevel::ERROR, "Disconnected: " + disconnect.reason);
    return false;
  }
  default:
    utils::log(utils::LogLevel::ERROR,
               "Unexpected packet ID after encryption: " +
                   std::to_string(packetId));
    return false;
  }
}

#include <chrono>
#include <thread>

int main() {
  try {
    AuthTokens tokens = performAuthentication();
    ServerConnection connection = getServerDetails();

    network::Socket socket;
    socket.connect(connection.address, connection.port);
    utils::log(utils::LogLevel::DEBUG,
               "Connected to server " + connection.address);

    mc::handler::PacketHandler handler(socket);
    handler.initialize(connection, tokens.minecraftUUID);
    handler.startReceiving();

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

  } catch (const std::exception &e) {
    utils::log(utils::LogLevel::ERROR, e.what());
    return 1;
  }
}
