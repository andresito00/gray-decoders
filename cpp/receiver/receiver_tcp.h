#ifndef DECODER_RECEIVER_RECEIVER_TCP_H_
#define DECODER_RECEIVER_RECEIVER_TCP_H_

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <util.h>
#include <concurrentqueue.h>
#include "receiver.h"

using namespace moodycamel;

class ReceiverTcp
{
 private:
  ReceiverStatus_e status_;
  size_t size_;
  std::string ip_;
  uint16_t port_;
  int bind_socket_;
  int comm_socket_;
  alignas(L1_CACHE_LINE_SIZE) uint8_t* buffer_;

  union {
    struct sockaddr_in server_address_;
    struct sockaddr server_address_alias_; // To avoid old-style casts
  };
  union {
    struct sockaddr_in client_address_;
    struct sockaddr client_address_alias_;
  };

  struct SpikeRaster deserialize(size_t num_bytes);

 public:
  ReceiverTcp(std::string ip, uint16_t port, size_t buffer_size);
  ~ReceiverTcp();

  // ReceiverTcp is a resource handle, so we should...
  ReceiverTcp(const ReceiverTcp& a) = delete;
  ReceiverTcp& operator=(const ReceiverTcp& a) = delete;
  ReceiverTcp(ReceiverTcp&& a) = delete;
  ReceiverTcp& operator=(ReceiverTcp&& a) = delete;

  ReceiverStatus_e initialize(void);
  // For now, this member function expects to receive on
  // spike raster structure boundaries. Should be made more robust.
  ReceiverStatus_e receive(ConcurrentQueue<struct SpikeRaster>& q);
  ReceiverStatus_e get_status(void);
  std::string get_ip(void);
  int get_port(void);
};

#endif  // DECODER_RECEIVER_RECEIVER_H_
