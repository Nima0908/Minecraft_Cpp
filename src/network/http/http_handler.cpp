#include "http_handler.hpp"
#include "../../util/logger.hpp"
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace mc::network::http {

HttpHandler::HttpHandler(net::io_context &ioc)
    : ioc_(ioc), sslCtx_(boost::asio::ssl::context::tlsv12_client) {

  sslCtx_.set_default_verify_paths();
  sslCtx_.set_verify_mode(boost::asio::ssl::verify_peer);
}

HttpHandler::UrlParts HttpHandler::parseUrl(const std::string &url) {
  UrlParts parts;

  auto scheme_pos = url.find("://");
  if (scheme_pos == std::string::npos) {
    mc::utils::log(mc::utils::LogLevel::ERROR, "Invalid URL format: ", url);
    throw std::runtime_error("Invalid URL format");
  }

  parts.scheme = url.substr(0, scheme_pos);
  auto remaining = url.substr(scheme_pos + 3);

  auto path_pos = remaining.find('/');
  std::string host_port = remaining.substr(0, path_pos);
  parts.target =
      path_pos != std::string::npos ? remaining.substr(path_pos) : "/";

  auto port_pos = host_port.find(':');
  if (port_pos != std::string::npos) {
    parts.host = host_port.substr(0, port_pos);
    parts.port = host_port.substr(port_pos + 1);
  } else {
    parts.host = host_port;
    parts.port = (parts.scheme == "https") ? "443" : "80";
  }

  return parts;
}

std::string HttpHandler::makeHttpsRequest(const std::string &host,
                                          const std::string &target,
                                          beast::http::verb verb,
                                          const std::string &body,
                                          const Headers &headers) {
  try {
    tcp::resolver resolver(ioc_);
    beast::ssl_stream<beast::tcp_stream> stream(ioc_, sslCtx_);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
      mc::utils::log(mc::utils::LogLevel::ERROR, "Failed to set SNI hostname");
      throw std::runtime_error("SSL SNI setup failed");
    }

    auto const results = resolver.resolve(host, "443");
    beast::get_lowest_layer(stream).connect(results);

    beast::get_lowest_layer(stream).expires_after(timeout_);
    stream.handshake(boost::asio::ssl::stream_base::client);

    beast::http::request<beast::http::string_body> req{verb, target, 11};
    req.set(beast::http::field::host, host);
    req.set(beast::http::field::user_agent, userAgent_);
    req.set(beast::http::field::connection, "close");

    for (const auto &[key, value] : headers) {
      req.set(key, value);
    }

    if (!body.empty()) {
      req.body() = body;
      req.prepare_payload();
    }

    mc::utils::log(mc::utils::LogLevel::DEBUG, "Sending HTTPS ",
                   beast::http::to_string(verb), " request to ", host, target);

    beast::get_lowest_layer(stream).expires_after(timeout_);
    beast::http::write(stream, req);

    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> res;
    beast::get_lowest_layer(stream).expires_after(timeout_);
    beast::http::read(stream, buffer, res);

    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Received HTTPS response with status: ", res.result_int());

    if (res.result() == beast::http::status::internal_server_error ||
        res.result() == beast::http::status::bad_gateway ||
        res.result() == beast::http::status::service_unavailable ||
        res.result() == beast::http::status::gateway_timeout) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "HTTPS server error: ", res.result_int());
      throw std::runtime_error("HTTPS server error: " +
                               std::to_string(res.result_int()));
    }

    beast::error_code ec;
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(10));
    stream.shutdown(ec);

    if (ec == net::error::eof ||
        ec == boost::asio::ssl::error::stream_truncated) {
      ec = {};
    } else if (ec && ec != beast::errc::not_connected) {
      mc::utils::log(mc::utils::LogLevel::WARN,
                     "SSL shutdown warning: ", ec.message());
    }

    return res.body();

  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "HTTPS request failed: ", e.what());
    throw;
  }
}

std::string
HttpHandler::makeHttpRequest(const std::string &host, const std::string &port,
                             const std::string &target, beast::http::verb verb,
                             const std::string &body, const Headers &headers) {
  try {
    tcp::resolver resolver(ioc_);
    beast::tcp_stream stream(ioc_);

    auto const results = resolver.resolve(host, port);
    stream.connect(results);

    beast::http::request<beast::http::string_body> req{verb, target, 11};
    req.set(beast::http::field::host, host);
    req.set(beast::http::field::user_agent, userAgent_);
    req.set(beast::http::field::connection, "close");

    for (const auto &[key, value] : headers) {
      req.set(key, value);
    }

    if (!body.empty()) {
      req.body() = body;
      req.prepare_payload();
    }

    mc::utils::log(mc::utils::LogLevel::DEBUG, "Sending HTTP ",
                   beast::http::to_string(verb), " request to ", host, ":",
                   port, target);

    stream.expires_after(timeout_);
    beast::http::write(stream, req);

    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> res;
    stream.expires_after(timeout_);
    beast::http::read(stream, buffer, res);

    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Received HTTP response with status: ", res.result_int());

    if (res.result() == beast::http::status::internal_server_error ||
        res.result() == beast::http::status::bad_gateway ||
        res.result() == beast::http::status::service_unavailable ||
        res.result() == beast::http::status::gateway_timeout) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "HTTP server error: ", res.result_int());
      throw std::runtime_error("HTTP server error: " +
                               std::to_string(res.result_int()));
    }

    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    if (ec && ec != beast::errc::not_connected) {
      mc::utils::log(mc::utils::LogLevel::WARN,
                     "HTTP shutdown warning: ", ec.message());
    }

    return res.body();

  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "HTTP request failed: ", e.what());
    throw;
  }
}

std::string HttpHandler::get(const std::string &url, const Headers &headers) {
  auto parts = parseUrl(url);

  if (parts.scheme == "https") {
    return makeHttpsRequest(parts.host, parts.target, beast::http::verb::get,
                            "", headers);
  } else if (parts.scheme == "http") {
    return makeHttpRequest(parts.host, parts.port, parts.target,
                           beast::http::verb::get, "", headers);
  } else {
    throw std::runtime_error("Unsupported URL scheme: " + parts.scheme);
  }
}

std::string HttpHandler::post(const std::string &url, const std::string &body,
                              const Headers &headers) {
  auto parts = parseUrl(url);

  if (parts.scheme == "https") {
    return makeHttpsRequest(parts.host, parts.target, beast::http::verb::post,
                            body, headers);
  } else if (parts.scheme == "http") {
    return makeHttpRequest(parts.host, parts.port, parts.target,
                           beast::http::verb::post, body, headers);
  } else {
    throw std::runtime_error("Unsupported URL scheme: " + parts.scheme);
  }
}

std::string HttpHandler::put(const std::string &url, const std::string &body,
                             const Headers &headers) {
  auto parts = parseUrl(url);

  if (parts.scheme == "https") {
    return makeHttpsRequest(parts.host, parts.target, beast::http::verb::put,
                            body, headers);
  } else if (parts.scheme == "http") {
    return makeHttpRequest(parts.host, parts.port, parts.target,
                           beast::http::verb::put, body, headers);
  } else {
    throw std::runtime_error("Unsupported URL scheme: " + parts.scheme);
  }
}

std::string HttpHandler::del(const std::string &url, const Headers &headers) {
  auto parts = parseUrl(url);

  if (parts.scheme == "https") {
    return makeHttpsRequest(parts.host, parts.target,
                            beast::http::verb::delete_, "", headers);
  } else if (parts.scheme == "http") {
    return makeHttpRequest(parts.host, parts.port, parts.target,
                           beast::http::verb::delete_, "", headers);
  } else {
    throw std::runtime_error("Unsupported URL scheme: " + parts.scheme);
  }
}

} // namespace mc::network::http
