#ifndef DECODER_NET_TCP_H_
#define DECODER_NET_TCP_H_

#include <string>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <net_core.h>

class LinuxTCPCore
{
 public:
  LinuxTCPCore();
  LinuxTCPCore(LinuxTCPCore&& other);
  LinuxTCPCore& operator=(LinuxTCPCore&& other);
  ~LinuxTCPCore();
  ssize_t wait_and_receive(unsigned char* buffer, size_t num_bytes) noexcept;
  netcore::NetCoreStatus get_status() const noexcept;
  void wait_for_connection();

 private:
  static constexpr size_t kMaxEvents = 10LU;
  netcore::NetCoreStatus status_;
  std::string ip_;
  uint16_t port_;
  netcore::file_descriptor_t bind_socket_;
  netcore::file_descriptor_t comm_socket_;
  netcore::file_descriptor_t epoll_fd_;
  struct epoll_event ev_;
  struct epoll_event events_[kMaxEvents];
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
