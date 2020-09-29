#ifndef DECODER_RECEIVER_RECEIVER_H_
#define DECODER_RECEIVER_RECEIVER_H_
#include <util.h>

typedef enum ReceiverStatus {
  RECEIVER_STATUS_ERROR = -1,
  RECEIVER_STATUS_OKAY = 0,
  RECEIVER_STATUS_CLOSED,
} ReceiverStatus_e;

class Receiver
{
 public:
  virtual ReceiverStatus_e initialize() = 0;
  virtual ReceiverStatus_e receive(SpikeRaster_t& result) = 0;
  virtual ReceiverStatus_e close() = 0;
  virtual ReceiverStatus_e get_status() = 0;
  virtual ~Receiver() {}
};

#endif  // __DECODER_RECEIVER_RECEIVER_H
