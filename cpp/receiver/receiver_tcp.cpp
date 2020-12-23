#include <iostream>
#include <vector>
#include <string>
#include <string.h>
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
#include "concurrentqueue.h"

ReceiverTcp::ReceiverTcp(std::string ip, uint16_t port, size_t buffer_size)
{
  ip_ = ip;
  port_ = port;

  ASSERT(buffer_size % 4 == 0);
  size_ = buffer_size;
}

ReceiverTcp::~ReceiverTcp()
{
  int ret = close(comm_socket_);
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  ret = close(bind_socket_);
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  delete[] buffer_;
}

ReceiverStatus_e ReceiverTcp::initialize()
{
  bind_socket_ = socket(AF_INET, SOCK_STREAM, 0);

  if (bind_socket_ == 0) {
    std::cout << strerror(errno) << std::endl;
    return RECEIVER_STATUS_ERROR;
  }

  int opt = 1;
  int ret =
      setsockopt(bind_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    return RECEIVER_STATUS_ERROR;
  }

  server_address_.sin_family = AF_INET;
  server_address_.sin_port = htons(port_);
  inet_pton(server_address_.sin_family, ip_.c_str(),
            &(server_address_.sin_addr));

  ret =
      bind(bind_socket_, &server_address_alias_, sizeof(server_address_alias_));
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    return RECEIVER_STATUS_ERROR;
  }

  std::cout << "Listening..." << std::endl;
  ret = listen(bind_socket_, 1);
  if (ret < 0) {
    std::cout << strerror(errno) << std::endl;
    return RECEIVER_STATUS_ERROR;
  }

  socklen_t client_addr_len = sizeof(client_address_alias_);
  comm_socket_ = accept(bind_socket_, &client_address_alias_, &client_addr_len);
  if (comm_socket_ < 0) {
    std::cout << strerror(errno) << std::endl;
    return RECEIVER_STATUS_ERROR;
  }

  buffer_ = new uint8_t[size_];

  status_ = RECEIVER_STATUS_OKAY;
  return status_;
}

SpikeRaster_t ReceiverTcp::deserialize(size_t num_bytes)
{
  SpikeRaster_t result;
  uint8_t *end = buffer_ + num_bytes;
  uint8_t *tail = end;

  tail = util_find_packet_end(buffer_, end);
  ASSERT(tail != nullptr);

  uint64_t *id = reinterpret_cast<uint64_t *>(buffer_);
  uint64_t *raster = id + 1;
  uint64_t *raster_end = reinterpret_cast<uint64_t *>(tail);

  return {*id, std::vector<uint64_t>(raster, raster_end)};
}

ReceiverStatus_e ReceiverTcp::receive(
    moodycamel::ConcurrentQueue<SpikeRaster_t> &q)
{
  while (true) {
    ssize_t bytes_received = recv(comm_socket_, buffer_, size_, 0);
    if (bytes_received > 0) {
      auto raster = deserialize(static_cast<size_t>(bytes_received));
      q.enqueue(raster);
    } else if (bytes_received < 0) {
      std::cout << strerror(errno) << std::endl;
      return RECEIVER_STATUS_ERROR;
    }
  }

  return RECEIVER_STATUS_OKAY;
}

ReceiverStatus_e ReceiverTcp::get_status(void) { return status_; }

std::string ReceiverTcp::get_ip(void) { return ip_; }

int ReceiverTcp::get_port(void) { return port_; }
