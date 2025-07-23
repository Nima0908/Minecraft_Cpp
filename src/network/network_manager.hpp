#pragma once
#include "../util/logger.hpp"
#include "http/http_handler.hpp"
#include <boost/asio.hpp>
#include <memory>

namespace mc::network {

class NetworkManager {
public:
  using WorkGuard =
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

  NetworkManager() = default;
  ~NetworkManager() = default;

  // Non-copyable, movable
  NetworkManager(const NetworkManager &) = delete;
  NetworkManager &operator=(const NetworkManager &) = delete;
  NetworkManager(NetworkManager &&) = default;
  NetworkManager &operator=(NetworkManager &&) = default;

  void start(boost::asio::io_context &ioc);
  void stop();

  // Access to HTTP client
  mc::network::http::HttpHandler *getHttpHandler();

  // Status
  bool isRunning() const;

private:
  boost::asio::io_context *ioc_{nullptr};
  std::unique_ptr<WorkGuard> work_guard_;
  std::unique_ptr<mc::network::http::HttpHandler> http_handler_;
};

} // namespace mc::network
