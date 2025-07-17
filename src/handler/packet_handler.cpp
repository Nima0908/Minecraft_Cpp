#include "packet_handler.hpp"
#include "../buffer/read_buffer.hpp"
#include "../protocol/packet_registry.hpp"
#include "../util/logger.hpp"
#include "../util/uuid_utils.hpp" // for formatUUIDFromBytes if needed
#include "stages/handshake_handler.hpp"
#include "stages/login_handler.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <typeinfo>

namespace mc::handler {

// === Constructor / Destructor ===
PacketHandler::PacketHandler(mc::network::Socket &sock) : socket(sock) {}
PacketHandler::~PacketHandler() { stopReceiving(); }

// === Set Connection Info ===
void PacketHandler::initialize(const ServerConnection &conn,
                               const std::string &uuid,
                               const std::string &token) {
  this->connection = conn;
  this->minecraftUUID = uuid;
  this->mcToken = token;
}

// === Start/Stop Receiving Thread ===
void PacketHandler::startReceiving() {
  if (running.load())
    return;
  running.store(true);
  receiveThread = std::thread(&PacketHandler::receiveLoop, this);
}

void PacketHandler::stopReceiving() {
  running.store(false);
  if (receiveThread.joinable())
    receiveThread.join();
}

// === Receiving Loop ===
void PacketHandler::receiveLoop() {
  while (running.load()) {
    try {
      receivePacket();
    } catch (const std::exception &e) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Exception in receive loop: " + std::string(e.what()));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// === Public Entry Point ===
void PacketHandler::receivePacket() {
  switch (currentState) {
  case mc::protocol::PacketState::Handshaking:
    handleHandshake();
    return;

  case mc::protocol::PacketState::Login:
  case mc::protocol::PacketState::Status:
  case mc::protocol::PacketState::Play:
    handleGenericPacket();
    return;

  default:
    mc::utils::log(mc::utils::LogLevel::ERROR, "Unknown packet state.");
    return;
  }
}

// === HANDSHAKING STATE HANDLER ===
void PacketHandler::handleHandshake() {
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "In Handshaking state, delegating to HandshakeHandler");

  constexpr int32_t protocolVersion = 770; // Adjust per version

  mc::handler::stages::HandshakeHandler handshake;
  handshake.sendHandshakeAndLogin(protocolVersion, connection, minecraftUUID,
                                  socket);

  currentState = mc::protocol::PacketState::Login;
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Handshake complete. Switching to Login state.");
}

// === GENERIC PACKET HANDLER ===
void PacketHandler::handleGenericPacket() {
  auto receivedPacket = socket.recvPacket();
  if (receivedPacket.empty()) {
    mc::utils::log(mc::utils::LogLevel::ERROR, "Received empty packet");
    return;
  }

  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Received raw packet size: " +
                     std::to_string(receivedPacket.size()));

  mc::buffer::ReadBuffer read(receivedPacket);
  int packetId = read.readVarInt();

  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Parsed packet ID: " + std::to_string(packetId));

  auto packet = createPacketFromId(
      currentState, mc::protocol::PacketDirection::Clientbound, packetId);
  if (!packet)
    return;

  try {
    packet->read(read);
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Packet read error: " + std::string(e.what()));
    return;
  }

  mc::utils::log(mc::utils::LogLevel::WARN,
                 "Received packet of type: " +
                     std::string(typeid(*packet).name()));

  if (currentState == mc::protocol::PacketState::Login) {
    mc::handler::stages::LoginHandler login_handler;
    login_handler.determinePacket(socket, packet, mcToken, minecraftUUID);
  }
}

// === FACTORY LOOKUP ===
std::unique_ptr<mc::protocol::Packet>
PacketHandler::createPacketFromId(mc::protocol::PacketState state,
                                  mc::protocol::PacketDirection direction,
                                  uint8_t packetId) {

  auto stateIt = mc::protocol::packetFactoryRegistry.find(state);
  if (stateIt == mc::protocol::packetFactoryRegistry.end()) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Unknown protocol state in packetFactoryRegistry");
    return nullptr;
  }

  const auto &directionMap = stateIt->second;
  auto dirIt = directionMap.find(direction);
  if (dirIt == directionMap.end()) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Unknown packet direction in directionMap");
    return nullptr;
  }

  const auto &idMap = dirIt->second;
  auto packetIt = idMap.find(packetId);
  if (packetIt == idMap.end()) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Unknown packet ID: " + std::to_string(packetId));
    return nullptr;
  }

  return std::unique_ptr<mc::protocol::Packet>(packetIt->second());
}

} // namespace mc::handler
