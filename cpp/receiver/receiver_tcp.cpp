#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <util.h>
#include "receiver_tcp.h"
#include "receiver.h"


ReceiverTcp::ReceiverTcp(std::string ip, int port, size_t buffer_size)
{
  ip_ = ip;
  port_ = port;

  assert(buffer_size % 4 == 0);
  size_ = buffer_size;
}

ReceiverTcp::~ReceiverTcp()
{
   delete[] buffer_;
}

ReceiverStatus_e ReceiverTcp::initialize()
{
  socket_ = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_ == 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  int ret =
      setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  address_.sin_family = AF_INET;
  address_.sin_port = htons(port_);
  inet_pton(address_.sin_family, ip_.c_str(), &(address_.sin_addr));
  ret = bind(socket_, &address_alias_, sizeof(address_alias_));

  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  buffer_ = new uint8_t[size_];

  status_ = RECEIVER_STATUS_OKAY;
  return status_;
}

// For now, this member function expects to receive on
// spike raster structure boundaries. Should be made more robust.
ReceiverStatus_e ReceiverTcp::receive(SpikeRaster_t& result)
{
  while(true) {
    ssize_t num_bytes = recv(socket_, buffer_, size_, 0);
    if (num_bytes > 0) {
        (void) result;
    } else if (num_bytes < 0) {
     std::cout << strerror(errno) << std::endl;
    }
  }

  return RECEIVER_STATUS_OKAY;
}

ReceiverStatus_e ReceiverTcp::close() { return RECEIVER_STATUS_OKAY; }

ReceiverStatus_e ReceiverTcp::get_status(void) { return status_; }

std::string ReceiverTcp::get_ip(void) { return ip_; }

int ReceiverTcp::get_port(void) { return port_; }
