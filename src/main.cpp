#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#include "authenticate/auth_manager.hpp"
#include "buffer/write_buffer.hpp"
#include "network/network_manager.hpp"
#include "protocol/client/handshaking/handshake.hpp"
#include "protocol/client/status/status_request.hpp"
#include "util/log_level.hpp"
#include "util/logger.hpp"

namespace mc {

// Constants for the Minecraft client
constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";
constexpr const char *SERVER_ADDRESS = "mc.hypixel.net";
constexpr const char *SERVER_IP = "172.65.254.166";
constexpr const char *SERVER_PORT_STR = "25565";
constexpr const int TICKS_PER_SECOND = 20;
constexpr int SERVER_PORT = 25565;
constexpr int PROTOCOL_VERSION = 770;
constexpr int LOGIN_STATE = 1;
constexpr const char *TOKEN_FILE = "tokens.json";
constexpr std::chrono::milliseconds TICK_DURATION(1000 / TICKS_PER_SECOND);
std::string USERNAME;


class MinecraftClient {
public:
  enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Authenticating,
    Ready
  };

  MinecraftClient() 
    : should_stop_{false}, 
      connection_state_{ConnectionState::Disconnected} {}
  
  void run() {
    mc::utils::log(mc::utils::LogLevel::INFO, "Starting MinecraftClient...");

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
      }
    );

    TickLoop();
    waitForExit();
    stop();
  }

  void TickLoop() {
    while (!should_stop_) {
      auto tick_start = std::chrono::steady_clock::now();
      auto tcpHandler = networkMgr_.getTcpHandler();
      auto httpHandler = networkMgr_.getHttpHandler();

      // Process a single tick
      processTick();

      auto tick_end = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        tick_end - tick_start);
      
      if (elapsed < TICK_DURATION) {
        std::this_thread::sleep_for(TICK_DURATION - elapsed);
      } else {
        mc::utils::log(mc::utils::LogLevel::WARN,
                       "Tick took too long: " + std::to_string(elapsed.count()) +
                       "ms");
      }
    }
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
  void processTick() {
    auto tcpHandler = networkMgr_.getTcpHandler();
    auto httpHandler = networkMgr_.getHttpHandler();

    if (!tcpHandler || !httpHandler) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Required handlers not available");
      return;
    }

    switch (connection_state_) {
      case ConnectionState::Disconnected: {
        connection_state_ = ConnectionState::Connecting;
        break;
      }

      case ConnectionState::Authenticating: {
        mc::auth::AuthManager auth(CLIENT_ID, TOKEN_FILE, httpHandler);
        auth.authenticate();
        mc::utils::log(mc::utils::LogLevel::DEBUG, "Authenticated");
        
        connection_state_ = ConnectionState::Connected;
        break;
      }
      
      case ConnectionState::Connecting: {
        auto connection = tcpHandler->createConnection();
        if (!connection) {
          mc::utils::log(mc::utils::LogLevel::ERROR, "Failed to create connection");
          
          connection_state_ = ConnectionState::Authenticating;
          return;
          break;
        }

        connection->setErrorCallback([](const boost::system::error_code &ec) {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Connection error: " + ec.message());
        });
          break;
        };
      

      case ConnectionState::Connected: {
        connection->connect(
          SERVER_IP, SERVER_PORT_STR,
          [connection](const boost::system::error_code &ec) {
            if (ec) {
              mc::utils::log(mc::utils::LogLevel::ERROR,
                             "Failed to connect: " + ec.message());
              return;
            }

            mc::utils::log(mc::utils::LogLevel::INFO, "Connected to server");

            mc::buffer::WriteBuffer write;
            auto handshakePacket =
              mc::protocol::client::handshaking::HandshakePacket(
                PROTOCOL_VERSION, SERVER_ADDRESS, SERVER_PORT, LOGIN_STATE);
                connection->sendPacket(handshakePacket.serialize(write));
          }
        ); 
        connection_state_ = ConnectionState::Ready;
        break;
      }

      case ConnectionState::Ready: {
        auto statusRequestPacket =
          mc::protocol::client::status::StatusRequest();
          connection->startReceiving();

          connection->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
          mc::utils::log(mc::utils::LogLevel::INFO, "Data received!");

          mc::buffer::WriteBuffer write1;
          connection->sendPacket(statusRequestPacket.serialize(write1));

      }
     );
     break;
    }
   } 
  }

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

  ConnectionState connection_state_;
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
