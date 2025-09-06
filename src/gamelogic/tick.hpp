#include <atomic>
#include <string>
#include <thread>

#include "../authenticate/Shared.h"
#include "../authenticate/auth_manager.hpp"
#include "../buffer/write_buffer.hpp"
#include "../network/network_manager.hpp"
#include "../network/tcp/tcp_handler.hpp"
#include "../protocol/client/handshaking/handshake.hpp"
#include "../protocol/client/status/status_request.hpp"
#include "../util/log_level.hpp"
#include "../util/logger.hpp"


namespace mc::tickbase {
class TickProcess {
public:
  enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Authenticating,
    Ready
  };

  void processTick() {
    auto connection_state_ = ConnectionState::Disconnected;
    auto tcpHandler = networkMgr_.getTcpHandler();
    auto httpHandler = networkMgr_.getHttpHandler();

    if (!tcpHandler || !httpHandler) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Required handlers not available");
      return;
    }

    switch (connection_state_) {
    case ConnectionState::Disconnected: {
      mc::utils::log(mc::utils::LogLevel::DEBUG,
                     "State: Disconnected -> Connecting");
      connection_state_ = ConnectionState::Connecting;
      break;
    }

    case ConnectionState::Connecting: {
      if (!connection) {
        connection = tcpHandler->createConnection();
        if (!connection) {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Failed to create connection");
          // maybe try again next tick
          return;
        }

        connection->setErrorCallback([](const boost::system::error_code &ec) {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Connection error: " + ec.message());
        });

        connection->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
          mc::utils::log(mc::utils::LogLevel::INFO, "Data received!");
        });
      }

      connection->connect(
          SERVER_IP, SERVER_PORT_STR,
          [this](const boost::system::error_code &ec) {
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
          });
      connection_state_ = ConnectionState::Connected;
      break;
    }

    case ConnectionState::Connected: {
      // Now send the status request and start receiving data
      if (connection) {
        mc::buffer::WriteBuffer write;
        auto statusRequestPacket =
            mc::protocol::client::status::StatusRequest();

        connection->sendPacket(statusRequestPacket.serialize(write));

        connection->startReceiving();

        connection_state_ = ConnectionState::Ready;
      } else {
        mc::utils::log(mc::utils::LogLevel::ERROR, "No connection available");
      }
      break;
    }

    case ConnectionState::Ready: {
      // Here you could handle incoming packets or maintain connection
      // For example, set or handle data callback again if needed
      if (connection) {
        connection->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
          mc::utils::log(mc::utils::LogLevel::INFO, "Data received!");
          // Handle incoming data here...
        });
      }
      break;
    }

    case ConnectionState::Authenticating: {
      mc::auth::AuthManager auth(CLIENT_ID, TOKEN_FILE, httpHandler);
      auth.authenticate();
      mc::utils::log(mc::utils::LogLevel::DEBUG, "Authenticated");

      connection_state_ = ConnectionState::Connected;
      break;
    }
    }
  }

private:
  std::atomic<ConnectionState> connection_state_;
  std::atomic<bool> should_stop_;
  boost::asio::io_context ioc_;
  std::thread network_thread_;
  mc::network::NetworkManager networkMgr_;
  std::shared_ptr<mc::network::tcp::TcpConnection> connection;
};
void runTickProcess() {
  const auto TICK_DURATION = std::chrono::milliseconds(50); // 20 ticks per second

  while (!should_stop_) {
    auto tick_start = std::chrono::steady_clock::now();

    TickProcess().processTick();

    auto tick_end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        tick_end - tick_start);

    if (elapsed < TICK_DURATION) {
      std::this_thread::sleep_for(TICK_DURATION - elapsed);
    } else {
      mc::utils::log(
          mc::utils::LogLevel::WARN,
          "Tick took too long: " + std::to_string(elapsed.count()) + "ms");
    }
  }

}
}; // namespace mc::tickbase
