#ifndef DECODER_RUNTIME_CONFIG_H_
#define DECODER_RUNTIME_CONFIG_H_

#include <string>

namespace runtimeconfig
{

size_t get_rx_buffer_size() noexcept;
void set_rx_buffer_size(size_t sz) noexcept;
std::string get_listen_ip() noexcept;
void set_listen_ip(std::string ip) noexcept;
uint16_t get_listen_port() noexcept;
void set_listen_port(unsigned short port) noexcept;

};  // namespace runtimeconfig

#endif  // DECODER_RUNTIME_CONFIG_H_
