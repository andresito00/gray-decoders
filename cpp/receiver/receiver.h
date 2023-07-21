#ifndef DECODER_RECEIVER_RECEIVER_H_
#define DECODER_RECEIVER_RECEIVER_H_
#include <string>
#include <cstddef>
#include <vector>
#include <array>
#include <climits>
#include <string.h>
#include <util.h>
#include <core.h>

enum class ReceiverStatus {
  kError = -1,
  kOkay,
  kStopped,
};

constexpr int kFailLimit = 5;
constexpr std::array<unsigned char, 4> kDelimiter{ 0xEF, 0xBE, 0xAD, 0xDE };

template<class T, class Q>
class Receiver
{
 public:
  ~Receiver() {}
  // ReceiverTcp is a resource handle, so we should...
  Receiver(Receiver&& a) = delete;
  Receiver(const Receiver& a) = delete;
  Receiver& operator=(Receiver&& a) = delete;
  Receiver& operator=(const Receiver& a) = delete;

  explicit Receiver(size_t buffer_size) :
      size_(buffer_size),
      stop_rx_(false),
      fail_count_(0)
  {
    net_core_ = T();
    rx_buffer_.reserve(size_);
    if (NetCoreStatus::kOkay == net_core_.get_status()) {
      status_ = ReceiverStatus::kOkay;
    } else {
      status_ = ReceiverStatus::kError;
    }
  }

  // For now, this member function expects to receive on
  // spike raster structure boundaries. Should be made more robust.
  ReceiverStatus Receive(Q &q) {
    status_ = ReceiverStatus::kOkay;
    while (!stop_rx_) {
      ssize_t bytes_received = net_core_.Receive(rx_buffer_, size_);
      if (bytes_received > 0) {
        auto raster = deserialize(bytes_received);
        q.enqueue(raster);
      } else if (bytes_received < 0) {
        ++fail_count_;
        if (fail_count_ > kFailLimit) {
          status_ = ReceiverStatus::kError;
          break;
        }
      }
    }

    return ReceiverStatus::kStopped;
  }

  ReceiverStatus Stop(void) {
    stop_rx_ = true;
  }

  ReceiverStatus get_status(void) const { return status_; }

 private:
  T net_core_;
  ReceiverStatus status_;
  bool stop_rx_;
  int fail_count_;
  size_t size_;
  std::vector<unsigned char> rx_buffer_;

  struct SpikeRaster deserialize(size_t num_bytes)
  {
    // todo: read up on better c++ deserialization patterns...
    auto range_start = rx_buffer_.begin();
    auto range_end = range_start + num_bytes;
    auto found_it = std::search(
      range_start, range_end, kDelimiter.begin(), kDelimiter.end()
    );
    if (found_it == range_end) {
      return { INT_MAX, std::vector<uint64_t> {} };
    }

    uint64_t id = uint64_t(
      (rx_buffer_[0]) << 56 |
      (rx_buffer_[1]) << 48 |
      (rx_buffer_[2]) << 40 |
      (rx_buffer_[3]) << 32 |
      (rx_buffer_[4]) << 24 |
      (rx_buffer_[5]) << 16 |
      (rx_buffer_[6]) << 8 |
      (rx_buffer_[7])
    );

    return { id, std::vector<uint64_t>(rx_buffer_.begin() + sizeof(id), found_it) };
  }
};

#endif  // __DECODER_RECEIVER_RECEIVER_H
