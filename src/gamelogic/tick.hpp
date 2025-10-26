#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "../authenticate/Shared.h"
#include "../authenticate/auth_manager.hpp"
#include "../buffer/read_buffer.hpp"  
#include "../buffer/write_buffer.hpp"
#include "../network/http/http_handler.hpp"
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

  TickProcess(mc::network::tcp::TcpHandler* tcpHandler,
              mc::network::http::HttpHandler* httpHandler,
              std::shared_ptr<mc::network::tcp::TcpConnection> connection = nullptr)
      : tcpHandler_(tcpHandler),
        httpHandler_(httpHandler),
        connection_(connection),
        connection_state_(ConnectionState::Disconnected) {}


  void processTick() {
    if (!tcpHandler_ || !httpHandler_) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Required handlers not available -tick.hpp");
      return;
    }

    auto state = connection_state_.load();

    switch (state) {
    case ConnectionState::Disconnected: {
      mc::utils::log(mc::utils::LogLevel::DEBUG,
                     "State: Disconnected -> Connecting");
      connection_state_.store(ConnectionState::Connecting);
      break;
    }

    case ConnectionState::Connecting: {
      if (!connection_) {
        connection_ = tcpHandler_->createConnection();
        if (!connection_) {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Failed to create connection");
          return;
        }

        connection_->setErrorCallback([](const boost::system::error_code &ec) {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Connection error: " + ec.message());
        });

        connection_->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
          mc::utils::log(mc::utils::LogLevel::INFO, "Data received!");
        });
      }

      connection_->connect(
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

            connection_->sendPacket(handshakePacket.serialize(write));
          });
      connection_state_.store(ConnectionState::Authenticating);  
      break;
    }

    case ConnectionState::Connected: {
      if (connection_) {
        mc::buffer::WriteBuffer write;
        auto statusRequestPacket =
            mc::protocol::client::status::StatusRequest();

        connection_->sendPacket(statusRequestPacket.serialize(write));

        connection_->startReceiving();

        connection_state_.store(ConnectionState::Ready);
      } else {
        mc::utils::log(mc::utils::LogLevel::ERROR, "No connection available");
      }
      break;
    }

    case ConnectionState::Ready: {
      if (connection_) {
        connection_->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
          mc::utils::log(mc::utils::LogLevel::INFO, "Data received!");
            // we need to handle the data here.(im too dumb to do that)
        });
      }
      break;
    }

    case ConnectionState::Authenticating: {
      mc::auth::AuthManager auth(CLIENT_ID, TOKEN_FILE, httpHandler_);
      auth.authenticate();
      mc::utils::log(mc::utils::LogLevel::DEBUG, "Authenticated");

      connection_state_.store(ConnectionState::Connected);
      break;
    }
    }
  }

private:
  mc::network::tcp::TcpHandler* tcpHandler_;
  mc::network::http::HttpHandler* httpHandler_;
  std::shared_ptr<mc::network::tcp::TcpConnection> connection_;
  std::atomic<ConnectionState> connection_state_;
};

void runTickProcess(mc::network::tcp::TcpHandler* tcpHandler,
                    mc::network::http::HttpHandler* httpHandler,
                    std::shared_ptr<mc::network::tcp::TcpConnection> connection = nullptr) {
  TickProcess tp(tcpHandler, httpHandler, connection);

  while (!should_stop_) {
    auto tick_start = std::chrono::steady_clock::now();

    tp.processTick();

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

}  // namespace mc::tickbase

// For anyone reading this file: This is bad code, feel free to improve it in any way you see fit.
