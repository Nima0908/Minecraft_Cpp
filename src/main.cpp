#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "network/network_manager.hpp"
#include "util/logger.hpp"

namespace mc {

class MinecraftClient {
public:
  MinecraftClient() : should_stop_{false} {}

  void run() {
    mc::utils::log(mc::utils::LogLevel::INFO, "MinecraftClient starting...");

    networkMgr_.start(ioc_);
    mc::utils::log(mc::utils::LogLevel::DEBUG, "NetworkManager started");

    network_thread_ = std::thread([this]() {
      mc::utils::log(mc::utils::LogLevel::INFO, "Networking thread started");
      try {
        ioc_.run();
        mc::utils::log(mc::utils::LogLevel::INFO, "IO context finished");
      } catch (const std::exception &e) {
        mc::utils::log(mc::utils::LogLevel::ERROR,
                       std::string("IO context error: ") + e.what());
      }
    });

    auto tcpHandler = networkMgr_.getTcpHandler();
    if (!tcpHandler) {
      mc::utils::log(mc::utils::LogLevel::ERROR, "TCP handler not available");
      return;
    }

    auto connection = tcpHandler->createConnection();

    connection->setErrorCallback([](const boost::system::error_code &ec) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Connection error: " + ec.message());
    });

    connection->setDataCallback([](mc::buffer::ReadBuffer &buffer) {
      mc::utils::log(mc::utils::LogLevel::INFO, "Data received!");
    });

    connection->connect(
        "127.0.0.1", "25565",
        [connection](const boost::system::error_code &ec) {
          if (ec) {
            mc::utils::log(mc::utils::LogLevel::ERROR,
                           "Failed to connect: " + ec.message());
            return;
          }

          mc::utils::log(mc::utils::LogLevel::INFO, "Connected!");

          connection->startReceiving();
        });

    waitForExit();

    stop();
  }

  void stop() {
    if (should_stop_.exchange(true))
      return;

    mc::utils::log(mc::utils::LogLevel::INFO, "Shutting down client");

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
