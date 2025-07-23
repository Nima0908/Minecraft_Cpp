#include "authenticate/auth_manager.hpp"
#include "network/network_manager.hpp"
#include "threading/thread_manager.hpp"
#include "util/log_level.hpp"
#include "util/logger.hpp"

#include <boost/asio/io_context.hpp>
#include <stdint.h>

int main(int argc, char *argv[]) {
  boost::asio::io_context ioc;
  mc::network::NetworkManager networkMgr;
  mc::threading::ThreadManager threadMgr;

  threadMgr.submitToDedicated("Networking", [&ioc]() { ioc.run(); });

  networkMgr.start(ioc);

  constexpr const char *CLIENT_ID = "757bb3b3-b7ca-4bcd-a160-c92e6379c263";

  auto *httpHandler = networkMgr.getHttpHandler();
  if (!httpHandler) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "HttpHandler not available from NetworkManager");
    return 1;
  }

  mc::auth::AuthManager auth_manager(CLIENT_ID, "tokens.json", httpHandler);

  if (auth_manager.authenticate()) {
    auth_manager.joinServer("server-hash");
  }

  return 0;
}
