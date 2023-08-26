#ifndef DECODER_NET_TCP_H_
#define DECODER_NET_TCP_H_

#include <vector>
#include <string>
#include <arpa/inet.h>
#include <net_core.h>

class LinuxTCPCore
{
 public:
  LinuxTCPCore();
  ~LinuxTCPCore();
  ssize_t receive(unsigned char *buffer, size_t num_bytes);
  NetCoreStatus get_status(void);

 protected:
 private:
  NetCoreStatus status_;
  std::string ip_;
  uint16_t port_;
  int bind_socket_;
  int comm_socket_;
  union {
    struct sockaddr_in server_address_;
    struct sockaddr server_address_alias_;
  };
  union {
    struct sockaddr_in client_address_;
    struct sockaddr client_address_alias_;
  };
};

#endif  // DECODER_NET_TCP_H_
