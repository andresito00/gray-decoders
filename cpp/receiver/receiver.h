#ifndef DECODER_RECEIVER_RECEIVER_H_
#define DECODER_RECEIVER_RECEIVER_H_
#include <cstddef>
#include <vector>
#include <array>
#include <string.h>
#include <raster.h>
#include <net_core.h>

using SpikeRaster64 = raster::SpikeRaster64;

namespace receiver
{
enum class ReceiverStatus {
  kError = -1,
  kOkay,
  kStopped,
};

constexpr int kFailLimit = 5;

template <class T, class Q, class S = SpikeRaster64>
class Receiver
{
 public:
  ~Receiver() {}
  Receiver(Receiver&& a) = delete;
  Receiver(const Receiver& a) = delete;
  Receiver& operator=(Receiver&& a) = delete;
  Receiver& operator=(const Receiver& a) = delete;

  explicit Receiver(size_t buffer_size)
      : stop_rx_(false), fail_count_(0), size_(buffer_size)
  {
    // net_core_ = T();
    rx_buffer_.reserve(size_);
    if (NetCoreStatus::kOkay == net_core_.get_status()) {
      status_ = ReceiverStatus::kOkay;
    } else {
      status_ = ReceiverStatus::kError;
    }
  }

  // For now, this member function expects to receive on
  // spike raster structure boundaries. Should be made more robust.
  ReceiverStatus receive(Q& q)
  {
    status_ = ReceiverStatus::kOkay;
    while (!stop_rx_) {  // Can also do && net_core_.get_status() ==
                         // NetCoreStatus::kOkay
      size_t populated_bytes = rx_buffer_.size();
      rx_buffer_.resize(size_);

      ssize_t bytes_received = net_core_.receive(
          rx_buffer_.data() + populated_bytes, size_ - populated_bytes);

      if (bytes_received > 0) {
        if (static_cast<size_t>(bytes_received) < size_) {
          rx_buffer_.resize(static_cast<size_t>(bytes_received));
        }
        std::vector<S> rasters;
        size_t bytes_deserialized = S::deserialize(rx_buffer_, rasters);
        q.enqueue_bulk(rasters.begin(),
                       rasters.size());  // use batch enqueue...
        rx_buffer_.erase(rx_buffer_.begin(),
                         rx_buffer_.begin() + bytes_deserialized);

      } else if (bytes_received < 0) {
        if (++fail_count_ > kFailLimit) {
          status_ = ReceiverStatus::kError;
          break;
        }
      }
    }

    return ReceiverStatus::kStopped;
  }

  ReceiverStatus stop(void) { stop_rx_ = true; }

  ReceiverStatus get_status(void) const { return status_; }

 private:
  T net_core_;
  ReceiverStatus status_;
  bool stop_rx_;
  size_t fail_count_;
  size_t size_;
  std::vector<unsigned char> rx_buffer_;
};

};  // namespace receiver

#endif  // __DECODER_RECEIVER_RECEIVER_H
