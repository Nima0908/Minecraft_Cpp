#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "authenticate/auth_manager.hpp"
#include "buffer/write_buffer.hpp"
#include "network/network_manager.hpp"
#include "protocol/client/handshaking/handshake.hpp"
#include "protocol/client/status/status_request.hpp"
#include "util/log_level.hpp"
#include "util/logger.hpp"

namespace mc {

constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";
constexpr const char *SERVER_ADDRESS = "localhost";
constexpr const char *SERVER_IP = "127.0.0.1";
constexpr const char *SERVER_PORT_STR = "25565";
constexpr int SERVER_PORT = 25565;
constexpr int PROTOCOL_VERSION = 770;
constexpr int LOGIN_STATE = 1;
constexpr const char *TOKEN_FILE = "tokens.json";
std::string USERNAME;

class MinecraftClient {
public:
  MinecraftClient() : should_stop_{false} {}

  void run() {
    mc::utils::log(mc::utils::LogLevel::INFO, "Starting MinecraftClient...");

    std::cout << "Username: ";
    std::cin >> USERNAME;

    networkMgr_.start(ioc_);
    mc::utils::log(mc::utils::LogLevel::DEBUG, "NetworkManager started");

    network_thread_ = std::thread([this]() {
      mc::utils::log(mc::utils::LogLevel::INFO, "Networking thread running");
      try {
        ioc_.run();
        mc::utils::log(mc::utils::LogLevel::INFO, "IO context exited normally");
      } catch (const std::exception &e) {
        mc::utils::log(mc::utils::LogLevel::ERROR,
                       std::string("IO context error: ") + e.what());
      }
    });

    auto tcpHandler = networkMgr_.getTcpHandler();
    auto httpHandler = networkMgr_.getHttpHandler();

    if (!tcpHandler || !httpHandler) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Required handlers not available");
      return;
    }

    mc::auth::AuthManager auth(CLIENT_ID, TOKEN_FILE, httpHandler);
    auth.authenticate();
    mc::utils::log(mc::utils::LogLevel::DEBUG, "Authenticated");

    auto connection = tcpHandler->createConnection();
    if (!connection) {
      mc::utils::log(mc::utils::LogLevel::ERROR, "Failed to create connection");
      return;
    }

    connection->setErrorCallback([](const boost::system::error_code &ec) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Connection error: " + ec.message());
    });

    connection->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
      mc::utils::log(mc::utils::LogLevel::INFO, "Data received!");
    });

    connection->connect(
        SERVER_IP, SERVER_PORT_STR,
        [connection](const boost::system::error_code &ec) {
          if (ec) {
            mc::utils::log(mc::utils::LogLevel::ERROR,
                           "Failed to connect: " + ec.message());
            return;
          }

          mc::utils::log(mc::utils::LogLevel::INFO, "Connected to server");

          connection->startReceiving();

          mc::buffer::WriteBuffer write;
          auto handshakePacket =
              mc::protocol::client::handshaking::HandshakePacket(
                  PROTOCOL_VERSION, SERVER_ADDRESS, SERVER_PORT, LOGIN_STATE);

          auto statusRequestPacket =
              mc::protocol::client::status::StatusRequest();

          connection->sendPacket(handshakePacket.serialize(write));

          mc::buffer::WriteBuffer write1;
          connection->sendPacket(statusRequestPacket.serialize(write1));
        });

    waitForExit();
    stop();
  }

  void stop() {
    if (should_stop_.exchange(true))
      return;

    mc::utils::log(mc::utils::LogLevel::INFO, "Shutting down client...");

    ioc_.stop();
    if (network_thread_.joinable())
      network_thread_.join();

    networkMgr_.stop();

    mc::utils::log(mc::utils::LogLevel::INFO, "MinecraftClient exited cleanly");
  }

private:
  void waitForExit() {
    mc::utils::log(mc::utils::LogLevel::INFO, "Press Ctrl+C to exit");

    static std::atomic<bool> signal_received{false};

    std::signal(SIGINT, [](int) {
      signal_received = true;
      mc::utils::log(mc::utils::LogLevel::INFO, "SIGINT received");
    });

    while (!should_stop_ && !signal_received) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }

  std::atomic<bool> should_stop_;
  boost::asio::io_context ioc_;
  std::thread network_thread_;
  mc::network::NetworkManager networkMgr_;
};

} // namespace mc

int main() {
  mc::MinecraftClient client;
  client.run();
  return 0;
}
