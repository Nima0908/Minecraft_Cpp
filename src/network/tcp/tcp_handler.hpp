#pragma once

#include "../../buffer/read_buffer.hpp"
#include "../../crypto/aes_cipher.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>

namespace mc::network::tcp {

using mc::buffer::ByteArray;
using mc::buffer::ReadBuffer;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
  using ConnectCallback =
      std::function<void(const boost::system::error_code &)>;
  using DataCallback = std::function<void(ReadBuffer &)>;
  using ErrorCallback = std::function<void(const boost::system::error_code &)>;

  static constexpr std::size_t BUFFER_SIZE = 8192;

  explicit TcpConnection(boost::asio::io_context &ioc);
  ~TcpConnection();

  void connect(const std::string &host, const std::string &port,
               ConnectCallback callback);
  void disconnect();
  bool isConnected() const { return connected_; }

  void send(const ByteArray &data);
  void send(const std::string &data);
  void sendPacket(const ByteArray &packet_data);

  void startReceiving();

  void enableEncryption(std::shared_ptr<mc::crypto::AESCipher> cipher);
  bool isEncryptionEnabled() const { return encryption_enabled_; }

  void setCompressionThreshold(int threshold);
  int getCompressionThreshold() const { return compression_threshold_; }

  void setTimeout(const std::chrono::milliseconds &timeout) {
    timeout_ = timeout;
  }
  void setKeepAlive(bool keep_alive) { keep_alive_ = keep_alive; }

  void setDataCallback(DataCallback callback) {
    data_callback_ = std::move(callback);
  }
  void setErrorCallback(ErrorCallback callback) {
    error_callback_ = std::move(callback);
  }

private:
  void doReceive();
  void handleConnect(const boost::system::error_code &error,
                     ConnectCallback callback);
  void handleSend(const boost::system::error_code &error,
                  std::size_t bytes_transferred);
  void handleReceive(const boost::system::error_code &error,
                     std::size_t bytes_transferred);
  void handleTimeout(const boost::system::error_code &error);
  void onError(const boost::system::error_code &error);
  void resetTimeout();

  ByteArray processOutgoingData(const ByteArray &data);
  ByteArray processIncomingData(const ByteArray &data);
  ByteArray compressIfNeeded(const ByteArray &data);
  ByteArray decompressIfNeeded(const ByteArray &data);

  boost::asio::ip::tcp::socket socket_;
  boost::asio::steady_timer timeout_timer_;
  std::vector<uint8_t> receive_buffer_;

  std::atomic<bool> connected_;
  bool keep_alive_;
  std::chrono::milliseconds timeout_;

  ConnectCallback connect_callback_;
  DataCallback data_callback_;
  ErrorCallback error_callback_;

  std::mutex mutex_;
  std::shared_ptr<mc::crypto::AESCipher> cipher_;
  bool encryption_enabled_;
  int compression_threshold_;
  ByteArray decrypted_buffer_;
};

class TcpHandler {
public:
  using ConnectionPtr = std::shared_ptr<TcpConnection>;

  explicit TcpHandler(boost::asio::io_context &ioc);
  ~TcpHandler();

  ConnectionPtr createConnection();

  void setDefaultTimeout(const std::chrono::milliseconds &timeout) {
    default_timeout_ = timeout;
  }
  void setDefaultKeepAlive(bool keep_alive) {
    default_keep_alive_ = keep_alive;
  }

  std::size_t getActiveConnections() const {
    return active_connections_.load();
  }

private:
  void onConnectionCreated();
  void onConnectionDestroyed();

  boost::asio::io_context &ioc_;
  std::chrono::milliseconds default_timeout_;
  bool default_keep_alive_;
  std::atomic<std::size_t> active_connections_;
};

} // namespace mc::network::tcp
