#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <chrono>
#include <map>
#include <string>

namespace mc::network::http {

class HttpHandler {
public:
  using Headers = std::map<std::string, std::string>;

  explicit HttpHandler(boost::asio::io_context &ioc);
  ~HttpHandler() = default;

  // Non-copyable, movable
  HttpHandler(const HttpHandler &) = delete;
  HttpHandler &operator=(const HttpHandler &) = delete;
  HttpHandler(HttpHandler &&) = default;
  HttpHandler &operator=(HttpHandler &&) = default;

  // HTTP methods
  std::string get(const std::string &url, const Headers &headers = {});
  std::string post(const std::string &url, const std::string &body = "",
                   const Headers &headers = {});
  std::string put(const std::string &url, const std::string &body = "",
                  const Headers &headers = {});
  std::string del(const std::string &url, const Headers &headers = {});

  // Configuration
  void setTimeout(std::chrono::seconds timeout) { timeout_ = timeout; }
  void setUserAgent(const std::string &userAgent) { userAgent_ = userAgent; }

private:
  boost::asio::io_context &ioc_;
  boost::asio::ssl::context sslCtx_;
  std::chrono::seconds timeout_{30};
  std::string userAgent_{"HttpHandler/1.0"};

  struct UrlParts {
    std::string scheme;
    std::string host;
    std::string port;
    std::string target;
  };

  UrlParts parseUrl(const std::string &url);
  std::string makeHttpsRequest(const std::string &host,
                               const std::string &target,
                               boost::beast::http::verb verb,
                               const std::string &body, const Headers &headers);
  std::string makeHttpRequest(const std::string &host, const std::string &port,
                              const std::string &target,
                              boost::beast::http::verb verb,
                              const std::string &body, const Headers &headers);
};

} // namespace mc::network::http
