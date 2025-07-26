#include "tcp_handler.hpp"
#include "../../buffer/read_buffer.hpp"
#include "../../buffer/write_buffer.hpp"
#include "../../util/compression_util.hpp"
#include "../../util/logger.hpp"

namespace mc::network::tcp {

using mc::buffer::ByteArray;
using mc::buffer::ReadBuffer;
using mc::buffer::WriteBuffer;

TcpConnection::TcpConnection(boost::asio::io_context &ioc)
    : socket_(ioc), timeout_timer_(ioc), resolver_(ioc),
      receive_buffer_(BUFFER_SIZE), connected_(false), keep_alive_(false),
      timeout_(std::chrono::seconds(30)), encryption_enabled_(false),
      compression_threshold_(-1) {}

TcpConnection::~TcpConnection() { disconnect(); }

void TcpConnection::connect(const std::string &host, const std::string &port,
                            ConnectCallback callback) {
  if (connected_) {
    callback(boost::system::error_code{});
    return;
  }

  connect_callback_ = std::move(callback);

  resetTimeout();

  auto self = shared_from_this();

  mc::utils::log(mc::utils::LogLevel::DEBUG, "Resolving host...");

  resolver_.async_resolve(
      host, port,
      [this,
       self](const boost::system::error_code &error,
             const boost::asio::ip::tcp::resolver::results_type &endpoints) {
        if (error) {
          mc::utils::log(mc::utils::LogLevel::ERROR,
                         "Resolve failed: " + error.message());
          timeout_timer_.cancel();
          handleConnect(error, connect_callback_);
          return;
        }

        mc::utils::log(mc::utils::LogLevel::DEBUG,
                       "Host resolved: attempting connect...");

        boost::asio::async_connect(
            socket_, endpoints,
            [this, self](const boost::system::error_code &error,
                         const boost::asio::ip::tcp::endpoint &ep) {
              if (!error) {
                mc::utils::log(mc::utils::LogLevel::DEBUG,
                               "Connect completed to " +
                                   ep.address().to_string());
              }
              timeout_timer_.cancel();
              handleConnect(error, connect_callback_);
            });
      });
}

void TcpConnection::disconnect() {
  if (!connected_)
    return;

  connected_ = false;
  timeout_timer_.cancel();
  resolver_.cancel();

  boost::system::error_code ec;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  socket_.close(ec);

  mc::utils::log(mc::utils::LogLevel::DEBUG, "TCP connection disconnected");
}

void TcpConnection::enableEncryption(
    std::shared_ptr<mc::crypto::AESCipher> cipher) {
  std::lock_guard<std::mutex> lock(mutex_);
  cipher_ = std::move(cipher);
  encryption_enabled_ = true;
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Encryption enabled for TCP connection");
}

void TcpConnection::setCompressionThreshold(int threshold) {
  std::lock_guard<std::mutex> lock(mutex_);
  compression_threshold_ = threshold;
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Compression threshold set to: " + std::to_string(threshold));
}

void TcpConnection::send(const ByteArray &data) {
  if (!connected_) {
    onError(boost::asio::error::not_connected);
    return;
  }

  try {
    ByteArray processed_data = processOutgoingData(data);
    auto self = shared_from_this();
    boost::asio::async_write(
        socket_, boost::asio::buffer(processed_data),
        [this, self](const boost::system::error_code &error,
                     std::size_t bytes_transferred) {
          handleSend(error, bytes_transferred);
        });
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to process outgoing data: " + std::string(e.what()));
    onError(boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument));
  }
}

void TcpConnection::sendPacket(const ByteArray &packet_data) {
  if (!connected_) {
    onError(boost::asio::error::not_connected);
    return;
  }

  try {
    ByteArray compressed = compressIfNeeded(packet_data);

    WriteBuffer buf;
    buf.writeVarInt(static_cast<int32_t>(compressed.size()));
    buf.writeBytes(compressed);
    ByteArray compiled = buf.compile();

    ByteArray final_data =
        encryption_enabled_ && cipher_ ? cipher_->encrypt(compiled) : compiled;

    auto self = shared_from_this();
    boost::asio::async_write(
        socket_, boost::asio::buffer(final_data),
        [this, self](const boost::system::error_code &error,
                     std::size_t bytes_transferred) {
          handleSend(error, bytes_transferred);
        });
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to send packet: " + std::string(e.what()));
    onError(boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument));
  }
}

void TcpConnection::send(const std::string &data) {
  ByteArray buffer(data.begin(), data.end());
  send(buffer);
}

void TcpConnection::startReceiving() {
  if (!connected_)
    return;
  doReceive();
}

void TcpConnection::doReceive() {
  if (!connected_)
    return;

  auto self = shared_from_this();
  socket_.async_read_some(boost::asio::buffer(receive_buffer_),
                          [this, self](const boost::system::error_code &error,
                                       std::size_t bytes_transferred) {
                            handleReceive(error, bytes_transferred);
                          });
}

ByteArray TcpConnection::processOutgoingData(const ByteArray &data) {
  std::lock_guard<std::mutex> lock(mutex_);

  ByteArray processed = compressIfNeeded(data);

  if (encryption_enabled_ && cipher_) {
    processed = cipher_->encrypt(processed);
  }

  return processed;
}

ByteArray TcpConnection::processIncomingData(const ByteArray &data) {
  std::lock_guard<std::mutex> lock(mutex_);

  decrypted_buffer_.insert(decrypted_buffer_.end(), data.begin(), data.end());

  ByteArray processed;
  if (encryption_enabled_ && cipher_) {
    processed = cipher_->decrypt(decrypted_buffer_);
    decrypted_buffer_.clear();
  } else {
    processed = decrypted_buffer_;
    decrypted_buffer_.clear();
  }

  return decompressIfNeeded(processed);
}

ByteArray TcpConnection::compressIfNeeded(const ByteArray &data) {
  if (compression_threshold_ < 0) {
    return data;
  }

  WriteBuffer buf;

  if (static_cast<int>(data.size()) >= compression_threshold_) {
    ByteArray compressed = mc::utils::compress(data);
    buf.writeVarInt(static_cast<int32_t>(data.size()));
    buf.writeBytes(compressed);
  } else {
    buf.writeVarInt(0);
    buf.writeBytes(data);
  }

  return buf.compile();
}

ByteArray TcpConnection::decompressIfNeeded(const ByteArray &data) {
  if (compression_threshold_ < 0) {
    return data;
  }

  ReadBuffer buf(data);
  int32_t uncompressed_length = buf.readVarInt();

  if (uncompressed_length == 0) {
    return buf.readBytes(buf.remaining());
  }

  ByteArray compressed_data = buf.readBytes(buf.remaining());
  ByteArray decompressed = mc::utils::decompress(compressed_data);

  if (static_cast<int32_t>(decompressed.size()) != uncompressed_length) {
    throw std::runtime_error("Decompressed size mismatch");
  }

  return decompressed;
}

void TcpConnection::handleConnect(const boost::system::error_code &error,
                                  ConnectCallback callback) {
  if (error) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "TCP connection failed: " + error.message());
    onError(error);
    if (callback)
      callback(error);
    return;
  }

  connected_ = true;

  if (keep_alive_) {
    boost::asio::socket_base::keep_alive option(true);
    boost::system::error_code ec;
    socket_.set_option(option, ec);
  }

  mc::utils::log(mc::utils::LogLevel::INFO, "TCP connection established");

  if (callback)
    callback(error);
}

void TcpConnection::handleSend(const boost::system::error_code &error,
                               std::size_t bytes_transferred) {
  if (error) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "TCP send failed: " + error.message());
    onError(error);
    return;
  }

  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "TCP sent " + std::to_string(bytes_transferred) + " bytes");
}

void TcpConnection::handleReceive(const boost::system::error_code &error,
                                  std::size_t bytes_transferred) {
  if (error) {
    if (error != boost::asio::error::eof) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "TCP receive failed: " + error.message());
    }
    onError(error);
    return;
  }

  if (bytes_transferred > 0 && data_callback_) {
    try {
      ByteArray raw_data(receive_buffer_.begin(),
                         receive_buffer_.begin() + bytes_transferred);
      ByteArray processed_data = processIncomingData(raw_data);

      if (!processed_data.empty()) {
        ReadBuffer buffer(std::move(processed_data));
        data_callback_(buffer);
      }
    } catch (const std::exception &e) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Failed to process incoming data: " +
                         std::string(e.what()));
    }
  }

  doReceive();
}

void TcpConnection::handleTimeout(const boost::system::error_code &error) {
  if (error == boost::asio::error::operation_aborted) {
    return;
  }

  mc::utils::log(mc::utils::LogLevel::WARN, "TCP connection timeout");
  onError(boost::asio::error::timed_out);
}

void TcpConnection::onError(const boost::system::error_code &error) {
  if (error_callback_) {
    error_callback_(error);
  }
  disconnect();
}

void TcpConnection::resetTimeout() {
  if (timeout_.count() <= 0)
    return;

  timeout_timer_.cancel();
  timeout_timer_.expires_after(timeout_);

  auto self = shared_from_this();
  timeout_timer_.async_wait(
      [this, self](const boost::system::error_code &error) {
        handleTimeout(error);
      });
}

// TcpHandler Implementation
TcpHandler::TcpHandler(boost::asio::io_context &ioc)
    : ioc_(ioc), default_timeout_(std::chrono::seconds(30)),
      default_keep_alive_(false), active_connections_(0) {
  mc::utils::log(mc::utils::LogLevel::INFO, "TCP handler initialized");
}

TcpHandler::~TcpHandler() {
  mc::utils::log(mc::utils::LogLevel::INFO, "TCP handler destroyed");
}

TcpHandler::ConnectionPtr TcpHandler::createConnection() {
  auto connection = std::make_shared<TcpConnection>(ioc_);
  connection->setTimeout(default_timeout_);
  connection->setKeepAlive(default_keep_alive_);

  connection->setErrorCallback(
      [this](const boost::system::error_code &) { onConnectionDestroyed(); });

  onConnectionCreated();
  return connection;
}

void TcpHandler::onConnectionCreated() {
  ++active_connections_;
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "TCP connection created. Active: " +
                     std::to_string(active_connections_.load()));
}

void TcpHandler::onConnectionDestroyed() {
  --active_connections_;
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "TCP connection destroyed. Active: " +
                     std::to_string(active_connections_.load()));
}

} // namespace mc::network::tcp
