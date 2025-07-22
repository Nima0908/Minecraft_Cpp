// http_client.hpp
#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <string>
#include <vector>

namespace mc::auth {
    using Headers = std::vector<std::pair<std::string, std::string>>;
    
    class HttpClient {
    private:
        boost::asio::io_context ioc;
        boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23_client};
        
    public:
        HttpClient();
        std::string post(const std::string& url, const std::string& body, const Headers& headers = {});
        std::string get(const std::string& url, const Headers& headers = {});
        
    private:
        std::pair<std::string, std::string> parseUrl(const std::string& url);
        std::string makeRequest(const std::string& host, const std::string& target, 
                              boost::beast::http::verb verb, const std::string& body = "", 
                              const Headers& headers = {});
    };
}
