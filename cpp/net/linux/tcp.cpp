#include <vector>
#include <string>
#include <string.h>
#include <algorithm>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <runtime_config.h>
#include <log.h>
#include <net_core.h>
#include "tcp.h"

LinuxTCPCore::LinuxTCPCore(void)
{
  ip_ = runtimeconfig::get_listen_ip();
  port_ = runtimeconfig::get_listen_port();
  bind_socket_ = socket(AF_INET, SOCK_STREAM, 0);

  if (bind_socket_ == 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }

  int opt = 1;
  if (setsockopt(bind_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }
  // int flags = fcntl(bind_socket_, F_GETFL);
  // if (flags < 0) {
  //   status_ = NetCoreStatus::kError;
  //   throw NetException(LOG_STRING(strerror(errno)));
  // }
  // if (fcntl(bind_socket_, F_SETFL, flags | O_NONBLOCK) < 0) {
  //   status_ = NetCoreStatus::kError;
  //   throw NetException(LOG_STRING(strerror(errno)));
  // }

  server_address_.sin_family = AF_INET;
  server_address_.sin_port = htons(port_);
  inet_pton(server_address_.sin_family, ip_.c_str(),
            &(server_address_.sin_addr));

  if (bind(bind_socket_, &server_address_alias_,
           sizeof(server_address_alias_)) < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }

  if (listen(bind_socket_, 1) < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }

  // We're interested in events on our listener socket, so we register it.
  ev_.events = EPOLLIN;
  ev_.data.fd = bind_socket_;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, bind_socket_, &ev_) < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }

  LOG("Listening...");

  int nfds = epoll_wait(epoll_fd_, events_, kMaxEvents, -1);
  if (nfds < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }
  socklen_t client_addr_len = sizeof(client_address_alias_);
  for (size_t i = 0; i < static_cast<size_t>(nfds); ++i) {
    if (events_[i].data.fd == bind_socket_) {
      // We've picked up a request to connect on our listener so we create a
      // connection.
      comm_socket_ =
          accept(bind_socket_, &client_address_alias_, &client_addr_len);
      if (comm_socket_ < 0) {
        status_ = NetCoreStatus::kError;
        throw NetException(LOG_STRING(strerror(errno)));
      }
      ev_.events = EPOLLIN;
      ev_.data.fd = comm_socket_;
      if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, comm_socket_, &ev_) < 0) {
        status_ = NetCoreStatus::kError;
        throw NetException(LOG_STRING(strerror(errno)));
      }
      status_ = NetCoreStatus::kOkay;
      LOG("comm_socket_ added with epoll_ctl...");
    }
  }
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

ssize_t LinuxTCPCore::receive(unsigned char *buffer, size_t num_bytes) noexcept
{
  if (num_bytes == 0) {
    return 0;
  }
  int nfds = epoll_wait(epoll_fd_, events_, kMaxEvents, -1);
  if (nfds < 0) {
    status_ = NetCoreStatus::kError;
  }
  ssize_t bytes_received = 0;
  for (size_t i = 0; i < static_cast<size_t>(nfds); ++i) {
    if (events_[i].data.fd == comm_socket_) {
      bytes_received = recv(comm_socket_, buffer, num_bytes, 0);
      if (bytes_received < 0) {
        status_ = NetCoreStatus::kError;
      }
      status_ = NetCoreStatus::kOkay;
    }
  }
  return bytes_received;
}

NetCoreStatus LinuxTCPCore::get_status(void) const noexcept { return status_; }
