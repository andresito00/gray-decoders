#ifndef DECODER_NET_NET_CORE_H_
#define DECODER_NET_NET_CORE_H_

#include <string>
#include <stdexcept>

namespace netcore
{

using file_descriptor_t = int;

static constexpr file_descriptor_t kInvalidFd = -1;

enum class NetCoreStatus {
  kError = -1,
  kOkay,
  kClosed,
};

class NetException : public std::runtime_error
{
 public:
  NetException(const std::string& msg) : std::runtime_error{msg}, message_{msg}
  {
  }
  NetException(std::string&& msg) : std::runtime_error{msg}, message_{msg} {}
  NetException(const char* msg) : std::runtime_error{msg}, message_{msg} {}
  const char* what() const noexcept override { return message_.c_str(); }

 private:
  std::string message_;
};

};  // namespace netcore

#endif  // DECODER_NET_NET_CORE_H_
