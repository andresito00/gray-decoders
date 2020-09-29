#ifndef DECODER_RECEIVER_RECEIVER_TCP_H_
#define DECODER_RECEIVER_RECEIVER_TCP_H_

#include "receiver.h"
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class ReceiverTcp : public Receiver
{
 private:
  receiver_status_e status;
  std::string ip;
  int port;
  union {
    // helps avoid old-style casts
    struct sockaddr_in address;
    struct sockaddr address_alias;
  };
  size_t size;
  uint8_t *buffer;

 public:
  ReceiverTcp(std::string ip, int port, size_t buffer_size)
      : ip{ip}, port{port}, size{buffer_size}
  {
  }
  ~ReceiverTcp() { delete[] buffer; }

  receiver_status_e initialize() override
  {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == 0) {
      std::cout << strerror(errno) << std::endl;
      exit(EXIT_FAILURE);
    }

    int opt = 1;
    int ret =
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
      std::cout << strerror(errno) << std::endl;
      exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(address.sin_addr));
    ret = bind(server_fd, &address_alias, sizeof(address));

    if (ret < 0) {
      std::cout << strerror(errno) << std::endl;
      exit(EXIT_FAILURE);
    }

    buffer = new uint8_t[size];

    status = RECEIVER_STATUS_OKAY;
    return status;
  }

  receiver_status_e start() override { return RECEIVER_STATUS_OKAY; }

  receiver_status_e close() override { return RECEIVER_STATUS_OKAY; }

  receiver_status_e get_status(void) override { return status; }

  std::string get_ip(void) { return ip; }

  int get_port(void) { return port; }
};

#endif  // DECODER_RECEIVER_RECEIVER_H_
