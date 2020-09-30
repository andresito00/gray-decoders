#ifndef DECODER_RECEIVER_RECEIVER_TCP_H_
#define DECODER_RECEIVER_RECEIVER_TCP_H_

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <util.h>
#include "receiver.h"


class ReceiverTcp : public Receiver
{
 private:
  ReceiverStatus_e status_;
  alignas(L1_CACHE_LINE_SIZE) uint8_t *buffer_;
  size_t size_;
  std::string ip_;
  int port_;
  int bind_socket_;
  int comm_socket_;
  // helps avoid old-style casts
  union {
    struct sockaddr_in server_address_;
    struct sockaddr server_address_alias_;
  };
  union {
    struct sockaddr_in client_address_;
    struct sockaddr client_address_alias_;
  };

 public:
  ReceiverTcp(std::string ip, int port, size_t buffer_size);
  ~ReceiverTcp();
  ReceiverStatus_e initialize(void) override;

  // For now, this member function expects to receive on
  // spike raster structure boundaries. Should be made more robust.
  ReceiverStatus_e receive(SpikeRaster_t& result) override;
  ReceiverStatus_e close() override;
  ReceiverStatus_e get_status(void) override;
  std::string get_ip(void);
  int get_port(void);
};

#endif  // DECODER_RECEIVER_RECEIVER_H_
