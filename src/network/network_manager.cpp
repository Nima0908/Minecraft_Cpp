#include "network_manager.hpp"
#include "../util/logger.hpp"

namespace mc::network {

void NetworkManager::start(boost::asio::io_context &ioc) {
  ioc_ = &ioc;
  work_guard_ = std::make_unique<WorkGuard>(boost::asio::make_work_guard(ioc));

  // Initialize HTTP handler (existing)
  http_handler_ = std::make_unique<mc::network::http::HttpHandler>(ioc);
  http_handler_->setTimeout(std::chrono::seconds(30));
  http_handler_->setUserAgent("MinecraftClient/1.0");

  // Initialize TCP handler (new)
  tcp_handler_ = std::make_unique<mc::network::tcp::TcpHandler>(ioc);
  tcp_handler_->setDefaultTimeout(std::chrono::seconds(30));
  tcp_handler_->setDefaultKeepAlive(true);

  mc::utils::log(mc::utils::LogLevel::INFO,
                 "NetworkManager started with HTTP and TCP client support");
}

void NetworkManager::stop() {
  mc::utils::log(mc::utils::LogLevel::INFO, "Stopping NetworkManager");

  // Clean up handlers
  http_handler_.reset();
  tcp_handler_.reset(); // New cleanup

  if (work_guard_) {
    work_guard_->reset();
    work_guard_.reset();
  }
  ioc_ = nullptr;

  mc::utils::log(mc::utils::LogLevel::INFO, "NetworkManager stopped");
}

mc::network::http::HttpHandler *NetworkManager::getHttpHandler() {
  return http_handler_.get();
}

mc::network::tcp::TcpHandler *NetworkManager::getTcpHandler() {
  return tcp_handler_.get();
}

bool NetworkManager::isRunning() const {
  return ioc_ != nullptr && work_guard_ != nullptr;
}

} // namespace mc::network
