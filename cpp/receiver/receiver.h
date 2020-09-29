#ifndef DECODER_RECEIVER_RECEIVER_H_
#define DECODER_RECEIVER_RECEIVER_H_

typedef enum receiver_status {
  RECEIVER_STATUS_ERROR = -1,
  RECEIVER_STATUS_OKAY = 0,
  RECEIVER_STATUS_CLOSED,
} receiver_status_e;

class Receiver {
 public:
  virtual receiver_status_e initialize() = 0;
  virtual receiver_status_e start() = 0;
  virtual receiver_status_e close() = 0;
  virtual receiver_status_e get_status() = 0;
  virtual ~Receiver() {}
};

#endif  // __DECODER_RECEIVER_RECEIVER_H
