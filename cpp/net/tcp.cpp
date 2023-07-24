#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "tcp.h"
#include "net_core.h"

static inline void log(std::string file, int line, char *msg) {
    std::cout << file << ":" << line << ": " <<  msg << std::endl;
}

ssize_t LinuxTCPCore::Receive(unsigned char *buffer, size_t num_bytes) {
    // TODO: Use select
    ssize_t bytes_received = recv(comm_socket_, buffer, num_bytes, 0);
    if (bytes_received > 0 && (bytes_received == static_cast<size_t>(bytes_received))) {
        status_ = NetCoreStatus::kOkay;
    } else if (bytes_received < 0) {
        status_ = NetCoreStatus::kError;
    }
    return bytes_received;
}

LinuxTCPCore::LinuxTCPCore(void) {
    ip_ = "0.0.0.0";
    port_ = 8808;
    bind_socket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (bind_socket_ == 0) {
      log(__FILE__, __LINE__,  strerror(errno));
      status_ = NetCoreStatus::kError;
      return;
    }

    int opt = 1;
    int ret =
        setsockopt(bind_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
      log(__FILE__, __LINE__,  strerror(errno));
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
      log(__FILE__, __LINE__,  strerror(errno));
      status_ = NetCoreStatus::kError;
      return;
    }

    log(__FILE__, __LINE__, "Listening...");
    ret = listen(bind_socket_, 1);
    if (ret < 0) {
      log(__FILE__, __LINE__,  strerror(errno));
      status_ = NetCoreStatus::kError;
      return;
    }

    socklen_t client_addr_len = sizeof(client_address_alias_);
    comm_socket_ = accept(bind_socket_, &client_address_alias_, &client_addr_len);
    if (comm_socket_ < 0) {
      log(__FILE__, __LINE__,  strerror(errno));
      status_ = NetCoreStatus::kError;
      return;
    }
    log(__FILE__, __LINE__, "Socket ready...");
    status_ = NetCoreStatus::kOkay;
}

LinuxTCPCore::~LinuxTCPCore(void) {
    int ret = close(comm_socket_);
    if (ret < 0) {
        log(__FILE__, __LINE__,  strerror(errno));
        status_ = NetCoreStatus::kError;
    }

    ret = close(bind_socket_);
    if (ret < 0) {
        log(__FILE__, __LINE__,  strerror(errno));
        status_ = NetCoreStatus::kError;
    }
}

NetCoreStatus LinuxTCPCore::get_status(void) {
    return status_;
}