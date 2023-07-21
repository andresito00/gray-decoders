#include <vector>
#include <string>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "tcp.h"

ssize_t LinuxTCPCore::Receive(std::vector<unsigned char>& buffer, size_t num_bytes) {
    // TODO: Use select
    buffer.resize(std::min(num_bytes, buffer.size()));
    ssize_t bytes_received = recv(comm_socket_, buffer.data(), buffer.size(), 0);
    if (bytes_received > 0) {
        buffer.resize(std::min(static_cast<size_t>(bytes_received), buffer.size()));
        status_ = NetCoreStatus::kOkay;
    } else if (bytes_received < 0) {
        status_ = NetCoreStatus::kError;
    }
    return bytes_received;
}

LinuxTCPCore::LinuxTCPCore(void) {
    ip_ = "127.0.0.1";
    port_ = 8080;
    bind_socket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (bind_socket_ == 0) {
      // std::cout << strerror(errno) << std::endl;
      status_ = NetCoreStatus::kError;
      return;
    }

    int opt = 1;
    int ret =
        setsockopt(bind_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
      // std::cout << strerror(errno) << std::endl;
      status_ = NetCoreStatus::kError;
      return;
    }

    server_address_.sin_family = AF_INET;
    server_address_.sin_port = htons(port_);
    inet_pton(server_address_.sin_family, ip_.c_str(),
              &(server_address_.sin_addr));

    ret =
        bind(bind_socket_, &server_address_alias_, sizeof(server_address_alias_));
    if (ret < 0) {
      // std::cout << strerror(errno) << std::endl;
      status_ = NetCoreStatus::kError;
      return;
    }

    // std::cout << "Listening..." << std::endl;
    ret = listen(bind_socket_, 1);
    if (ret < 0) {
      // std::cout << strerror(errno) << std::endl;
      status_ = NetCoreStatus::kError;
      return;
    }

    socklen_t client_addr_len = sizeof(client_address_alias_);
    comm_socket_ = accept(bind_socket_, &client_address_alias_, &client_addr_len);
    if (comm_socket_ < 0) {
      // std::cout << strerror(errno) << std::endl;
      status_ = NetCoreStatus::kError;
      return;
    }

    status_ = NetCoreStatus::kOkay;
}

LinuxTCPCore::~LinuxTCPCore(void) {
    int ret = close(comm_socket_);
    if (ret < 0) {
      // std::cout << strerror(errno) << std::endl;
        status_ = NetCoreStatus::kError;
    }

    ret = close(bind_socket_);
    if (ret < 0) {
      // std::cout << strerror(errno) << std::endl;
        status_ = NetCoreStatus::kError;
    }
}

NetCoreStatus LinuxTCPCore::get_status(void) {
    return status_;
}