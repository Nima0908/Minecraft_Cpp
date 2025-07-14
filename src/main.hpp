#pragma once

#include "network/socket.hpp"
#include <functional>
#include <string>

constexpr int DEFAULT_PORT = 25565;
constexpr int PROTOCOL_VERSION = 770;
constexpr int LOGIN_STATE = 2;

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

// Authentication
AuthTokens performAuthentication();
std::string getOrCreateToken(const std::string &filename,
                             std::function<std::string()> createToken);

// Server setup
ServerConnection getServerDetails();

// Network logic
void sendHandshakeAndLogin(mc::network::Socket &socket,
                           const ServerConnection &connection,
                           const std::string &minecraftUUID);

bool processLoginPacket(mc::network::Socket &socket, const std::string &mcToken,
                        const std::string &minecraftUUID);

bool processPostEncryptionPackets(mc::network::Socket &socket);
