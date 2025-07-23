#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>

namespace mc::network::http {

class Session : public std::enable_shared_from_this<Session> {
public:
  explicit Session(boost::asio::ip::tcp::socket socket);
  void start();

private:
  void doRead();
  void handleRequest();
  void doWrite();

  boost::asio::ip::tcp::socket socket_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::request<boost::beast::http::string_body> req_;
  boost::beast::http::response<boost::beast::http::string_body> res_;
};

} // namespace mc::network::http
