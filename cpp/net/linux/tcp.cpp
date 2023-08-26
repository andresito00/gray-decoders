#include <vector>
#include <string>
#include <string.h>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <log.h>
#include <net_core.h>
#include "tcp.h"

LinuxTCPCore::LinuxTCPCore(void)
{
  ip_ = "0.0.0.0";
  port_ = 8808;
  bind_socket_ = socket(AF_INET, SOCK_STREAM, 0);

  if (bind_socket_ == 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
    return;
  }

  int opt = 1;
  int ret =
      setsockopt(bind_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
    return;
  }

  server_address_.sin_family = AF_INET;
  server_address_.sin_port = htons(port_);
  inet_pton(server_address_.sin_family, ip_.c_str(),
            &(server_address_.sin_addr));

  ret =
      bind(bind_socket_, &server_address_alias_, sizeof(server_address_alias_));
  if (ret < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
    return;
  }

  LOG("Listening...");
  ret = listen(bind_socket_, 1);
  if (ret < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
    return;
  }

  socklen_t client_addr_len = sizeof(client_address_alias_);
  comm_socket_ = accept(bind_socket_, &client_address_alias_, &client_addr_len);
  if (comm_socket_ < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
    return;
  }
  LOG("Socket ready...");
  status_ = NetCoreStatus::kOkay;
}

LinuxTCPCore::~LinuxTCPCore(void)
{
  int ret = close(comm_socket_);
  if (ret < 0) {
    status_ = NetCoreStatus::kError;
    LOG(strerror(errno));
  }

  ret = close(bind_socket_);
  if (ret < 0) {
    status_ = NetCoreStatus::kError;
    LOG(strerror(errno));
  }
}

ssize_t LinuxTCPCore::receive(unsigned char *buffer, size_t num_bytes)
{
  // TODO: Use select
  ssize_t bytes_received = recv(comm_socket_, buffer, num_bytes, 0);
  if (bytes_received > 0 &&
      (bytes_received == static_cast<size_t>(bytes_received))) {
    status_ = NetCoreStatus::kOkay;
  } else if (bytes_received < 0) {
    status_ = NetCoreStatus::kError;
  }
  return bytes_received;
}

NetCoreStatus LinuxTCPCore::get_status(void) { return status_; }
