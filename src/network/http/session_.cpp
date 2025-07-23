#include "session_.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace mc::network::http {

Session::Session(net::ip::tcp::socket socket) : socket_(std::move(socket)) {}

void Session::start() { doRead(); }

void Session::doRead() {
  auto self = shared_from_this();
  beast::http::async_read(socket_, buffer_, req_,
                          [self](beast::error_code ec, std::size_t) {
                            if (!ec)
                              self->handleRequest();
                          });
}

void Session::handleRequest() {
  res_.version(req_.version());
  res_.keep_alive(false);
  res_.result(beast::http::status::ok);
  res_.set(beast::http::field::server, "mc-server");
  res_.set(beast::http::field::content_type, "text/plain");
  res_.body() = "Hello from Boost.Beast!";
  res_.prepare_payload();
  doWrite();
}

void Session::doWrite() {
  auto self = shared_from_this();
  beast::http::async_write(
      socket_, res_, [self](beast::error_code ec, std::size_t) {
        self->socket_.shutdown(net::ip::tcp::socket::shutdown_send, ec);
      });
}

} // namespace mc::network::http
