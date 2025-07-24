#pragma once

#include "http/http_handler.hpp" // Your existing HTTP handler
#include "tcp/tcp_handler.hpp"   // New TCP handler
#include <boost/asio.hpp>
#include <memory>

namespace mc::network {

class NetworkManager {
public:
  using WorkGuard =
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

  NetworkManager() = default;
  ~NetworkManager() = default;

  void start(boost::asio::io_context &ioc);
  void stop();
  bool isRunning() const;

  // HTTP access (existing)
  mc::network::http::HttpHandler *getHttpHandler();

  // TCP access (new)
  mc::network::tcp::TcpHandler *getTcpHandler();

private:
  boost::asio::io_context *ioc_ = nullptr;
  std::unique_ptr<WorkGuard> work_guard_;

  // Handlers
  std::unique_ptr<mc::network::http::HttpHandler> http_handler_;
  std::unique_ptr<mc::network::tcp::TcpHandler> tcp_handler_; // New member
};

} // namespace mc::network
