#include <string>
#include "runtime_config.h"

namespace runtimeconfig
{

static struct RuntimeConfig {
  RuntimeConfig() : rx_buffer_sz_{0}, listen_ip_{""}, listen_port_{0xFFFF} {}
  ~RuntimeConfig() = default;
  RuntimeConfig(RuntimeConfig& other) = delete;
  RuntimeConfig& operator=(RuntimeConfig& other) = delete;
  RuntimeConfig(RuntimeConfig&& other) = delete;
  RuntimeConfig& operator=(RuntimeConfig&& other) = delete;
  size_t rx_buffer_sz_;
  std::string listen_ip_;
  uint16_t listen_port_;
} global_rc;

size_t get_rx_buffer_size() noexcept { return global_rc.rx_buffer_sz_; }

void set_rx_buffer_size(size_t sz) noexcept { global_rc.rx_buffer_sz_ = sz; }

std::string get_listen_ip() noexcept { return global_rc.listen_ip_; }

void set_listen_ip(std::string ip) noexcept
{
  // Check for valid IP?
  global_rc.listen_ip_ = ip;
}

uint16_t get_listen_port() noexcept { return global_rc.listen_port_; }

void set_listen_port(unsigned short port) noexcept
{
  global_rc.listen_port_ = port;
}

};  // namespace runtimeconfig
