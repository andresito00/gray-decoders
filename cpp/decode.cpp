#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <memory>
#include <tclap/CmdLine.h>
#include <util.h>
#include <receiver_tcp.h>
#include "concurrentqueue.h"

int main(int argc, char *argv[])
{
  std::string ip;
  uint16_t port;
  try {
    TCLAP::CmdLine cmd("CLI interface to launch the decoder", ' ', "0.0");
    TCLAP::ValueArg<std::string> arg_ip("i", "ip", "IP address to bind to",
                                        true, "", "string");
    TCLAP::ValueArg<uint16_t> arg_port("p", "port", "Port to listen on", true, 8080,
                                  "int");

    cmd.add(arg_ip);
    cmd.add(arg_port);

    cmd.parse(argc, argv);

    ip = arg_ip.getValue();
    port = arg_port.getValue();

    std::cout << "Binding to " << ip << ": " << port << std::endl;

  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId()
              << std::endl;
  }

  // main routine should eventually be:
  //  accessible runtime configuration given args/initialization input
  //  initialize receiver process
  const auto q = std::make_shared<moodycamel::ConcurrentQueue<SpikeRaster_t>>();

  pid_t pid = fork();
  if (pid == 0) {
    while (true) {
      SpikeRaster_t found;
      bool result = q->try_dequeue(found);
      if (result) {
        std::cout << std::hex << found.id << std::endl;
      }
    }
  } else if (pid > 0) {
    ReceiverTcp *receiver = new ReceiverTcp(ip, port, 4096);
    receiver->initialize();
    receiver->receive(q);
  } else {
    ASSERT(0);
  }

  //  initialize learner process
  //  initialize decoder process(es)

  return 0;
}
