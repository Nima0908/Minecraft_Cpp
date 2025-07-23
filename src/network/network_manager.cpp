#include "network_manager.hpp"

namespace mc::network {

void NetworkManager::start(boost::asio::io_context &ioc) {
  ioc_ = &ioc;
  work_guard_ = std::make_unique<WorkGuard>(boost::asio::make_work_guard(ioc));

  // Initialize HTTP client handler
  http_handler_ = std::make_unique<mc::network::http::HttpHandler>(ioc);

  // Configure HTTP handler
  http_handler_->setTimeout(std::chrono::seconds(30));
  http_handler_->setUserAgent("MinecraftClient/1.0");

  mc::utils::log(mc::utils::LogLevel::INFO,
                 "NetworkManager started with HTTP client support");
}

void NetworkManager::stop() {
  mc::utils::log(mc::utils::LogLevel::INFO, "Stopping NetworkManager");

  // Reset HTTP handler
  http_handler_.reset();

  if (work_guard_) {
    work_guard_->reset(); // Let io_context exit
    work_guard_.reset();  // Destroy the guard
  }

  ioc_ = nullptr;
  mc::utils::log(mc::utils::LogLevel::INFO, "NetworkManager stopped");
}

mc::network::http::HttpHandler *NetworkManager::getHttpHandler() {
  return http_handler_.get();
}

bool NetworkManager::isRunning() const {
  return ioc_ != nullptr && work_guard_ != nullptr;
}

} // namespace mc::network
