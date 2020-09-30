#include <iostream>
#include <vector>
#include <string>
#include <cstddef>
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
  bind_socket_ = socket(AF_INET, SOCK_STREAM, 0);

  if (bind_socket_ == 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  int ret =
      setsockopt(bind_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  server_address_.sin_family = AF_INET;
  server_address_.sin_port = htons(port_);
  inet_pton(server_address_.sin_family, ip_.c_str(), &(server_address_.sin_addr));

  ret = bind(bind_socket_, &server_address_alias_, sizeof(server_address_alias_));
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  ret = listen(bind_socket_, 1);
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  socklen_t client_addr_len = sizeof(client_address_alias_);
  comm_socket_ = accept(bind_socket_, &client_address_alias_, &client_addr_len);
  if (comm_socket_ < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  buffer_ = new uint8_t[size_];

  status_ = RECEIVER_STATUS_OKAY;
  return status_;
}

// For now, this member function expects to receive on
// spike raster structure boundaries. Should be made more robust.
// I do not plan to handle MTU-based splits yet.
ReceiverStatus_e ReceiverTcp::receive(SpikeRaster_t& result)
{
  while(true) {
    ssize_t num_bytes = recv(comm_socket_, buffer_, size_, 0);
    if (num_bytes > 0) {
      uint32_t *end = (uint32_t *)(buffer_ + num_bytes);
      uint32_t *tail = util_find_raster_end((uint32_t *)buffer_, end);
      // Presumptuous for the time being...
      if (tail != nullptr) {
        memcpy(&result, buffer_, sizeof(SpikeRasterHeader_t));
        result.raster.insert(
          result.raster.end(),
          (double *)(buffer_ + sizeof(SpikeRasterHeader_t)),
          (double *)tail);
        if (end == tail + 1) {
          return RECEIVER_STATUS_OKAY;
        } else {
          assert(0);
        }
      }
      break;
    } else if (num_bytes < 0) {
      std::cout << strerror(errno) << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  return RECEIVER_STATUS_OKAY;
}

ReceiverStatus_e ReceiverTcp::close() { return RECEIVER_STATUS_OKAY; }

ReceiverStatus_e ReceiverTcp::get_status(void) { return status_; }

std::string ReceiverTcp::get_ip(void) { return ip_; }

int ReceiverTcp::get_port(void) { return port_; }
