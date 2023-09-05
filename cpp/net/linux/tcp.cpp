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

using NetCoreStatus = netcore::NetCoreStatus;
using NetException = netcore::NetException;

/**
 * Creates and binds socket
 */
LinuxTCPCore::LinuxTCPCore()
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

  server_address_.sin_family = AF_INET;
  server_address_.sin_port = htons(port_);
  inet_pton(server_address_.sin_family, ip_.c_str(),
            &(server_address_.sin_addr));

  if (bind(bind_socket_, &server_address_alias_,
           sizeof(server_address_alias_)) < 0) {
    status_ = NetCoreStatus::kError;
    throw NetException(LOG_STRING(strerror(errno)));
  }
  status_ = NetCoreStatus::kClosed;
}

LinuxTCPCore::LinuxTCPCore(LinuxTCPCore&& other)
{
  if (this == &other) {
    return;
  }
  status_ = other.status_;
  ip_ = other.ip_;
  port_ = other.port_;
  bind_socket_ = other.bind_socket_;
  comm_socket_ = other.comm_socket_;
  epoll_fd_ = other.epoll_fd_;
  ev_ = other.ev_;

  other.status_ = NetCoreStatus::kError;
  other.ip_ = "";
  other.port_ = 0xFFFF;
  other.bind_socket_ = netcore::kInvalidFd;
  other.comm_socket_ = netcore::kInvalidFd;
  other.epoll_fd_ = netcore::kInvalidFd;
  other.ev_ = {};
}

LinuxTCPCore& LinuxTCPCore::operator=(LinuxTCPCore&& other)
{
  if (this == &other) {
    return *this;
  }
  status_ = other.status_;
  ip_ = other.ip_;
  port_ = other.port_;
  bind_socket_ = other.bind_socket_;
  comm_socket_ = other.comm_socket_;
  epoll_fd_ = other.epoll_fd_;
  ev_ = other.ev_;

  other.status_ = NetCoreStatus::kError;
  other.ip_ = "";
  other.port_ = 0xFFFF;
  other.bind_socket_ = netcore::kInvalidFd;
  other.comm_socket_ = netcore::kInvalidFd;
  other.epoll_fd_ = netcore::kInvalidFd;
  other.ev_ = {};
  return *this;
}

LinuxTCPCore::~LinuxTCPCore()
{
  status_ = NetCoreStatus::kClosed;
  if ((comm_socket_ != netcore::kInvalidFd) && ((comm_socket_) < 0)) {
    status_ = NetCoreStatus::kError;
    LOG_ERROR(strerror(errno));
  }

  if ((bind_socket_ != netcore::kInvalidFd) && ((bind_socket_) < 0)) {
    status_ = NetCoreStatus::kError;
    LOG_ERROR(strerror(errno));
  }

  if ((epoll_fd_ != netcore::kInvalidFd) && ((epoll_fd_) < 0)) {
    status_ = NetCoreStatus::kError;
    LOG_ERROR(strerror(errno));
  }
}

/**
 * Waits for an incoming connection and initializes the epoll instance
 * that we'll use for async commuincation, assigning sockets and events of
 * interest.
 */
void LinuxTCPCore::wait_for_connection()
{
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

  LOG("Listening for connection...");

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
      // We'd like our socket to be non-blocking.
      int flags = fcntl(comm_socket_, F_GETFL);
      if (fcntl(comm_socket_, F_SETFL, flags | O_NONBLOCK) < 0) {
        status_ = NetCoreStatus::kError;
        throw NetException(LOG_STRING(strerror(errno)));
      }
      ev_.events = EPOLLIN | EPOLLET;
      ev_.events = EPOLLIN;
      ev_.data.fd = comm_socket_;
      if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, comm_socket_, &ev_) < 0) {
        status_ = NetCoreStatus::kError;
        throw NetException(LOG_STRING(strerror(errno)));
      }
      status_ = NetCoreStatus::kOkay;
    }
  }
  if (status_ == NetCoreStatus::kOkay) {
    LOG("Connection accpeted, stream open...");
  }
}

/**
 * Sleeps the thread until data is available on the socket.
 * Reads data into the buffer parameter for later deserialization.
 */
ssize_t LinuxTCPCore::wait_and_receive(unsigned char* buffer,
                                       size_t num_bytes) noexcept
{
  if (num_bytes == 0) {
    return 0;
  }
  int nfds = epoll_wait(epoll_fd_, events_, kMaxEvents, -1);
  if (nfds < 0) {
    status_ = NetCoreStatus::kError;
    return 0;
  }
  ssize_t bytes_received = 0;
  ssize_t curr_received = 0;

  // TODO: remove for loop as we don't expect to receive data
  // on multiple sockets yet.
  // BUG: Not handling delimiter + 7 bytes corner case
  for (size_t i = 0; i < static_cast<size_t>(nfds); ++i) {
    if (events_[i].data.fd == comm_socket_) {
      curr_received = read(comm_socket_, buffer, num_bytes);
      // EPOLLET requires that we continue reading from the FD until
      // EWOULDBLOCK/EAGAIN
      if (curr_received < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          break;
        } else {
          status_ = NetCoreStatus::kError;
          return -1;
        }
      } else {
        buffer += curr_received;
        bytes_received += curr_received;
      }
    }
  }
  status_ = NetCoreStatus::kOkay;
  return bytes_received;
}

NetCoreStatus LinuxTCPCore::get_status(void) const noexcept { return status_; }
