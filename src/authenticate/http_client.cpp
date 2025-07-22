// http_client.cpp
#include "http_client.hpp"
#include "../util/logger.hpp"
#include <boost/beast/version.hpp>

namespace mc::auth {

HttpClient::HttpClient() {
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(boost::asio::ssl::verify_peer);
}

std::pair<std::string, std::string> HttpClient::parseUrl(const std::string& url) {
    auto pos = url.find("://");
    if (pos == std::string::npos) {
        mc::utils::log(mc::utils::LogLevel::ERROR, "Invalid URL format: ", url);
        throw std::runtime_error("Invalid URL format");
    }
    
    auto remaining = url.substr(pos + 3);
    auto slash_pos = remaining.find('/');
    
    std::string host = remaining.substr(0, slash_pos);
    std::string target = slash_pos != std::string::npos ? remaining.substr(slash_pos) : "/";
    
    return {host, target};
}

std::string HttpClient::makeRequest(const std::string& host, const std::string& target,
                                  boost::beast::http::verb verb, const std::string& body,
                                  const Headers& headers) {
    try {
        boost::asio::ip::tcp::resolver resolver(ioc);
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ctx);

        // Set SNI hostname
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
            mc::utils::log(mc::utils::LogLevel::ERROR, "Failed to set SNI hostname");
            throw std::runtime_error("SSL SNI setup failed");
        }

        // Resolve and connect
        auto const results = resolver.resolve(host, "443");
        boost::beast::get_lowest_layer(stream).connect(results);

        // Timeout for handshake
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        stream.handshake(boost::asio::ssl::stream_base::client);

        // Prepare HTTP request
        boost::beast::http::request<boost::beast::http::string_body> req{verb, target, 11};
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, "MinecraftClient/1.0");
        req.set(boost::beast::http::field::connection, "close");

        for (const auto& [key, value] : headers) {
            req.set(key, value);
        }

        if (!body.empty()) {
            req.body() = body;
            req.prepare_payload();
        }

        mc::utils::log(mc::utils::LogLevel::DEBUG, "Sending ", boost::beast::http::to_string(verb), " request to ", host, target);

        // Send request
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        boost::beast::http::write(stream, req);

        // Receive response
        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
        boost::beast::http::read(stream, buffer, res);

        mc::utils::log(mc::utils::LogLevel::DEBUG, "Received response with status: ", res.result_int());
        mc::utils::log(mc::utils::LogLevel::DEBUG, "Response body length: ", res.body().length());

        std::string debug_body = res.body().substr(0, std::min(res.body().length(), size_t(500)));
        mc::utils::log(mc::utils::LogLevel::DEBUG, "Response body (first 500 chars): ", debug_body);

        // Expected OAuth 400: authorization_pending
        if (res.result() == boost::beast::http::status::bad_request &&
            res.body().find("authorization_pending") != std::string::npos) {
            
            static auto lastLogTime = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime).count() > 10) {
                mc::utils::log(mc::utils::LogLevel::DEBUG, "Waiting for user authorization (authorization_pending)");
                lastLogTime = now;
            }

            return res.body();
        }

        // Log other 400s or unexpected errors
        if (res.result() != boost::beast::http::status::ok &&
            res.result() != boost::beast::http::status::bad_request) {
            mc::utils::log(mc::utils::LogLevel::ERROR, "HTTP error: ", res.result_int(), " - ", res.body());
            throw std::runtime_error("HTTP request failed with status: " + std::to_string(res.result_int()));
        }

        // Graceful shutdown
        boost::beast::error_code ec;
        boost::beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(10));
        stream.shutdown(ec);

        if (ec == boost::asio::error::eof || ec == boost::asio::ssl::error::stream_truncated) {
            // Harmless SSL shutdown case
            mc::utils::log(mc::utils::LogLevel::DEBUG, "SSL shutdown: ", ec.message());
            ec = {};
        } else if (ec && ec != boost::beast::errc::not_connected) {
            mc::utils::log(mc::utils::LogLevel::WARN, "SSL shutdown warning: ", ec.message());
        }

        return res.body();

    } catch (const std::exception& e) {
        mc::utils::log(mc::utils::LogLevel::ERROR, "HTTP request failed: ", e.what());
        throw;
    }
}

std::string HttpClient::post(const std::string& url, const std::string& body, const Headers& headers) {
    auto [host, target] = parseUrl(url);
    return makeRequest(host, target, boost::beast::http::verb::post, body, headers);
}

std::string HttpClient::get(const std::string& url, const Headers& headers) {
    auto [host, target] = parseUrl(url);
    return makeRequest(host, target, boost::beast::http::verb::get, "", headers);
}

} // namespace mc::auth
