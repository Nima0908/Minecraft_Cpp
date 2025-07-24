#include "authenticate/auth_manager.hpp"
#include "network/network_manager.hpp"
#include "threading/thread_manager.hpp"
#include "util/log_level.hpp"
#include "util/logger.hpp"
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <stdint.h>
#include <string>
#include <thread>

int main(int argc, char *argv[]) {
  boost::asio::io_context ioc;
  mc::network::NetworkManager networkMgr;
  mc::threading::ThreadManager threadMgr;

  // Start the networking thread
  threadMgr.submitToDedicated("Networking", [&ioc]() {
    mc::utils::log(mc::utils::LogLevel::INFO, "Starting IO context");
    ioc.run();
  });

  networkMgr.start(ioc);

  constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";
  constexpr const char *SERVER_HOST = "localhost";
  constexpr int SERVER_PORT = 25565;

  // Get handlers
  auto *httpHandler = networkMgr.getHttpHandler();
  if (!httpHandler) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "HttpHandler not available from NetworkManager");
    return 1;
  }

  auto *tcpHandler = networkMgr.getTcpHandler();
  if (!tcpHandler) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "TcpHandler not available from NetworkManager");
    return 1;
  }

  // Authenticate with Microsoft/Mojang
  mc::auth::AuthManager auth_manager(CLIENT_ID, "tokens.json", httpHandler);
  if (!auth_manager.authenticate()) {
    mc::utils::log(mc::utils::LogLevel::ERROR, "Authentication failed");
    networkMgr.stop();
    threadMgr.stop();
    return 1;
  }

  mc::utils::log(mc::utils::LogLevel::INFO, "Authentication successful");

  // Join server (for session validation)
  /*if (auth_manager.joinServer("server-hash")) {
      mc::utils::log(mc::utils::LogLevel::WARN, "Failed to join server for
  session validation");
  }*/

  // Create TCP connection to Minecraft server
  auto connection = tcpHandler->createConnection();

  // Set up connection callbacks
  connection->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
    // mc::utils::log(mc::utils::LogLevel::INFO,
    //"Received " + std::to_string(buffer.data()) + " bytes from server");
    // TODO: Process Minecraft protocol packets here
  });

  connection->setErrorCallback([](const boost::system::error_code &error) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "TCP connection error: " + error.message());
  });

  // Connect to Minecraft server
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Connecting to Minecraft server at " +
                     std::string(SERVER_HOST) + ":" +
                     std::to_string(SERVER_PORT));

  bool connection_established = false;
  connection->connect(
      SERVER_HOST, std::to_string(SERVER_PORT),
      [&connection_established](const boost::system::error_code &error) {
        if (error) {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Failed to connect to server: " + error.message());
        } else {
          mc::utils::log(mc::utils::LogLevel::INFO,
                         "Successfully connected to Minecraft server");
          connection_established = true;
        }
      });

  // Wait for connection to establish or fail
  std::this_thread::sleep_for(std::chrono::seconds(2));

  if (connection_established) {
    mc::utils::log(mc::utils::LogLevel::INFO, "Starting packet reception");
    connection->startReceiving();

    // TODO: Send handshake packet, login packets, etc.
    // Example: Send a simple test message
    std::string test_message = "Hello Minecraft Server!";
    connection->send(test_message);

    // Keep connection alive for a bit to receive responses
    mc::utils::log(mc::utils::LogLevel::INFO,
                   "Maintaining connection for 30 seconds...");
    std::this_thread::sleep_for(std::chrono::seconds(30));

    mc::utils::log(mc::utils::LogLevel::INFO, "Disconnecting from server");
    connection->disconnect();
  } else {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to establish connection to server");
  }

  // Cleanup
  mc::utils::log(mc::utils::LogLevel::INFO, "Shutting down...");
  networkMgr.stop();
  threadMgr.stop();

  return connection_established ? 0 : 1;
}
