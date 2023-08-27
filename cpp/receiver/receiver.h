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
  Receiver(size_t buffer_size)
      : stop_rx_(false), fail_count_(0), size_(buffer_size)
  {
    rx_buffer_.reserve(size_);
    if (netcore::NetCoreStatus::kOkay == net_xport_.get_status()) {
      status_ = ReceiverStatus::kStopped;
    } else {
      status_ = ReceiverStatus::kError;
    }
  }
  ~Receiver() {}
  Receiver(const Receiver& a) = delete;
  Receiver& operator=(const Receiver& a) = delete;

  Receiver(Receiver&& other)
      : net_xport_{std::move(other.net_xport_)},
        status_{other.status_},
        stop_rx_{other.stop_rx_},
        fail_count_{other.fail_count_},
        size_{other.size_},
        rx_buffer_{std::move(other.rx_buffer_)}
  {
    if (this == &other) {
      return;
    }
    other.status_ = ReceiverStatus::kError;
    other.stop_rx_ = true;
    other.fail_count_ = kFailLimit;
    other.size_ = 0;
    other.rx_buffer_ = {};
  }

  Receiver& operator=(Receiver&& other)
  {
    if (this == &other) {
      return *this;
    }
    net_xport_ = std::move(other.net_xport_);
    status_ = other.status_;
    stop_rx_ = other.stop_rx_;
    fail_count_ = other.fail_count_;
    size_ = other.size_;
    rx_buffer_ = std::move(other.rx_buffer_);

    other.status_ = ReceiverStatus::kError;
    other.stop_rx_ = true;
    other.fail_count_ = kFailLimit;
    other.size_ = 0;
    other.rx_buffer_ = {};
    return *this;
  }

  ReceiverStatus net_initialize()
  {
    net_xport_.wait_for_connection();
    if (net_xport_.get_status() == netcore::NetCoreStatus::kOkay) {
      status_ = ReceiverStatus::kOkay;
    }
    return status_;
  }

  // For now, this member function expects to receive on
  // spike raster structure boundaries. Should be made more robust.
  ReceiverStatus receive(Q& q)
  {
    status_ = ReceiverStatus::kOkay;
    while (!stop_rx_) {  // Can also do && net_xport_.get_status() ==
                         // NetCoreStatus::kOkay
      size_t populated_bytes = rx_buffer_.size();
      rx_buffer_.resize(size_);

      ssize_t bytes_received = net_xport_.wait_and_receive(
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
  T net_xport_;
  ReceiverStatus status_;
  bool stop_rx_;
  size_t fail_count_;
  size_t size_;
  std::vector<unsigned char> rx_buffer_;
};

};  // namespace receiver

#endif  // __DECODER_RECEIVER_RECEIVER_H
