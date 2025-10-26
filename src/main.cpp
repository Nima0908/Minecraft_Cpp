#include <atomic>
#include <chrono>
#include <csignal>
#include <string>
#include <thread>

#include "authenticate/Shared.h"
#include "gamelogic/tick.hpp"
#include "authenticate/auth_manager.hpp"
#include "buffer/write_buffer.hpp"
#include "network/network_manager.hpp"
#include "network/tcp/tcp_handler.hpp"
#include "protocol/client/status/status_request.hpp"
#include "util/log_level.hpp"
#include "util/logger.hpp"

namespace mc {

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
      : connection_state_{ConnectionState::Disconnected} {}

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
                     "Required handlers not available -main.cpp");
      return;
    }


    tickbase::runTickProcess(tcpHandler, httpHandler);

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
