#include "packet_handler.hpp"
#include "../buffer/read_buffer.hpp"
#include "../protocol/packet_registry.hpp"
#include "../util/logger.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <typeinfo>

namespace mc::handler {

PacketHandler::PacketHandler(mc::network::Socket &sock) : socket(sock) {}

PacketHandler::~PacketHandler() { stopReceiving(); }

void PacketHandler::startReceiving() {
  if (running.load())
    return;
  running.store(true);
  receiveThread = std::thread(&PacketHandler::receiveLoop, this);
}

void PacketHandler::stopReceiving() {
  running.store(false);
  if (receiveThread.joinable()) {
    receiveThread.join();
  }
}

void PacketHandler::receiveLoop() {
  while (running.load()) {
    try {
      receivePacket(socket);
    } catch (const std::exception &e) {
      mc::utils::log(utils::LogLevel::ERROR,
                     std::string("Exception in receive loop: ") + e.what());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void PacketHandler::receivePacket(mc::network::Socket &socket) {
  auto receivedPacket = socket.recvPacket();
  mc::utils::log(utils::LogLevel::DEBUG,
                 "Received raw packet size: " +
                     std::to_string(receivedPacket.size()));

  if (receivedPacket.empty()) {
    mc::utils::log(utils::LogLevel::ERROR, "Received empty packet");
    return;
  }

  mc::buffer::ReadBuffer read(receivedPacket);
  int packetId = read.readVarInt();

  mc::utils::log(utils::LogLevel::DEBUG,
                 "Parsed packet ID: " + std::to_string(packetId));

  mc::protocol::PacketState state = currentState;
  auto direction = mc::protocol::PacketDirection::Clientbound;

  auto stateIt = mc::protocol::packetFactoryRegistry.find(state);
  if (stateIt == mc::protocol::packetFactoryRegistry.end()) {
    mc::utils::log(utils::LogLevel::ERROR,
                   "Unknown protocol state in packetFactoryRegistry");
    return;
  }

  auto &directionMap = stateIt->second;
  auto dirIt = directionMap.find(direction);
  if (dirIt == directionMap.end()) {
    mc::utils::log(utils::LogLevel::ERROR,
                   "Unknown packet direction in directionMap");
    return;
  }

  auto &idMap = dirIt->second;
  auto packetIt = idMap.find(static_cast<uint8_t>(packetId));
  if (packetIt == idMap.end()) {
    mc::utils::log(utils::LogLevel::ERROR,
                   "Unknown packet ID: " + std::to_string(packetId));
    return;
  }

  mc::protocol::PacketFactory factory = packetIt->second;
  std::unique_ptr<mc::protocol::Packet> packet(factory());

  try {
    packet->read(read);
  } catch (const std::exception &e) {
    mc::utils::log(utils::LogLevel::ERROR,
                   std::string("Packet read error: ") + e.what());
    return;
  }

  mc::utils::log(utils::LogLevel::WARN,
                 "Received packet of type: " +
                     std::string(typeid(*packet).name()));
}

} // namespace mc::handler
