#include <iostream>
#include <tclap/CmdLine.h>
#include <util.h>
#include <receiver_tcp.h>

int main(int argc, char *argv[])
{
  std::string ip;
  int port;
  try {
    TCLAP::CmdLine cmd("CLI interface to launch the decoder", ' ', "0.0");
    TCLAP::ValueArg<std::string> arg_ip("i", "ip", "IP address to bind to",
                                        true, "", "string");
    TCLAP::ValueArg<int> arg_port("p", "port", "Port to listen on", true, 8080,
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
  Receiver *receiver = new ReceiverTcp(ip, port, 4096);
  receiver->initialize();
  while(true) {
    SpikeRaster_t current;
    ReceiverStatus_e status = RECEIVER_STATUS_OKAY;
    status = receiver->receive(current);
    std::cout << current.header.num_events << std::endl;
    std::cout << current.header.epoch_ms << std::endl;
    std::cout << current.raster.size() << std::endl;
    return 0;

    if (status != RECEIVER_STATUS_OKAY) {
      exit(1);
    }

    // enqueue SpikeRaster_t current now that it has been populated
  }
  //  initialize learner process
  //  initialize decoder process(es)

  return 0;
}
