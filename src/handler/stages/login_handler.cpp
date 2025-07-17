#include "login_handler.hpp"

#include "../../authenticate/auth_session.hpp"
#include "../../buffer/read_buffer.hpp"
#include "../../crypto/aes_cipher.hpp"
#include "../../crypto/encryption.hpp"
#include "../../protocol/client/login/encryption_response.hpp"
#include "../../protocol/packet.hpp"
#include "../../protocol/server/login/encryption_request.hpp"
#include "../../protocol/server/login/login_compression.hpp"
#include "../../protocol/server/login/login_disconnect.hpp"
#include "../../protocol/server/login/login_finished.hpp"
#include "../../util/logger.hpp"
#include "../../util/uuid_utils.hpp"
#include "../packet_handler.hpp"
#include <ostream>
#include <string>
#include <sys/socket.h>

namespace mc::handler::stages {

void LoginHandler::determinePacket(
    mc::network::Socket &socket, std::unique_ptr<mc::protocol::Packet> &packet,
    const std::string &mcToken, const std::string &minecraftUUID) {
  using namespace mc::protocol::server::login;

  if (auto encryptionReq = dynamic_cast<EncryptionRequest *>(packet.get())) {
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Handling EncryptionRequest packet");
    handleEncryptionResponse(socket, packet, mcToken, minecraftUUID);
  } else if (auto loginFinished = dynamic_cast<LoginFinished *>(packet.get())) {
    mc::utils::log(mc::utils::LogLevel::DEBUG, "Handling LoginFinished packet");
    handleLoginAcknowledged(packet);
  } else if (auto loginCompression =
                 dynamic_cast<LoginCompression *>(packet.get())) {
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Handling LoginCompression packet");
    handleLoginCompression(socket, packet);
  } else if (auto loginDisconnect =
                 dynamic_cast<LoginDisconnect *>(packet.get())) {
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Handling LoginDisconnect packet");
    handleLoginDisconnect(socket, packet);
  } else {
    mc::utils::log(mc::utils::LogLevel::WARN,
                   "Unhandled login packet type received");
  }
}

void LoginHandler::handleEncryptionResponse(
    mc::network::Socket &socket, std::unique_ptr<mc::protocol::Packet> &packet,
    const std::string &mcToken, const std::string &minecraftUUID) {

  auto *encryptionRequest =
      dynamic_cast<protocol::server::login::EncryptionRequest *>(packet.get());
  if (!encryptionRequest) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Invalid packet cast to EncryptionRequest");
    return;
  }

  auto sharedSecret = mc::crypto::generateSharedSecret();
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Shared secret generated successfully.");
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Beginning RSA encryption. Data size: " +
                     std::to_string(sharedSecret.size()) +
                     ", Public key size: " +
                     std::to_string(encryptionRequest->publicKey.size()));

  auto encryptedSecret =
      mc::crypto::rsaEncrypt(sharedSecret, encryptionRequest->publicKey);
  auto encryptedToken = mc::crypto::rsaEncrypt(encryptionRequest->verifyToken,
                                               encryptionRequest->publicKey);

  std::string serverHash = mc::crypto::computeServerHash(
      encryptionRequest->serverID, sharedSecret, encryptionRequest->publicKey);

  mc::auth::sendJoinServerRequest(mcToken, minecraftUUID, serverHash);

  PacketHandler handler(socket);
  handler.sendPacket<mc::protocol::client::login::EncryptionResponse>(
      encryptedSecret, encryptedToken);
  socket.enableEncryption(std::make_shared<crypto::AESCipher>(sharedSecret));

  mc::utils::log(mc::utils::LogLevel::INFO, "AES encryption activated");
}

void LoginHandler::handleLoginCompression(
    mc::network::Socket &socket,
    std::unique_ptr<mc::protocol::Packet> &packet) {
  auto *loginCompression =
      dynamic_cast<protocol::server::login::LoginCompression *>(packet.get());
  if (!loginCompression) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Invalid packet cast to EncryptionRequest");
    return;
  }
  socket.setCompressionThreshold(loginCompression->threshold);
}

void LoginHandler::handleLoginAcknowledged(
    std::unique_ptr<mc::protocol::Packet> &packet) {
  auto *loginSuccess =
      dynamic_cast<protocol::server::login::LoginFinished *>(packet.get());
  if (!loginSuccess) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Invalid packet cast to EncryptionRequest");
    return;
  }
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Username: " + loginSuccess->username);
  std::string uuidStr = mc::utils::formatUUIDFromBytes(
      {loginSuccess->uuid.begin(), loginSuccess->uuid.end()});
  mc::utils::log(mc::utils::LogLevel::DEBUG, "UUID: " + uuidStr);

  for (const auto &prop : loginSuccess->properties) {
    std::string propLog = "Property: " + prop.name + " = " + prop.value;
    if (prop.signature.has_value()) {
      propLog += " (Signature: " + *prop.signature + ")";
    }
    utils::log(utils::LogLevel::DEBUG, propLog);
  }
}

void LoginHandler::handleLoginDisconnect(
    mc::network::Socket &socket,
    std::unique_ptr<mc::protocol::Packet> &packet) {
  auto *loginDisconnect =
      dynamic_cast<protocol::server::login::LoginDisconnect *>(packet.get());
  if (!loginDisconnect) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Invalid packet cast to EncryptionRequest");
    return;
  }
  mc::utils::log(mc::utils::LogLevel::ERROR,
                 "Disconnect packet recieved, stopping.\n Reason: " +
                     loginDisconnect->reason);
  mc::handler::PacketHandler handler(socket);
  handler.stopReceiving();
}

} // namespace mc::handler::stages
