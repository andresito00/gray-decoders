#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <memory>
#include <tclap/CmdLine.h>
#include <util.h>
#include <receiver_tcp.h>
#include "concurrentqueue.h"

int initialize(
  std::string ip,
  uint16_t port,
  moodycamel::ConcurrentQueue<SpikeRaster_t>& q)
{
  ReceiverTcp *receiver = new ReceiverTcp(ip, port, 4096);
  receiver->initialize();
  ReceiverStatus_e status = receiver->receive(q);
  if (status == RECEIVER_STATUS_ERROR) {
    std::cout << "huh?" << std::endl;
    exit(EXIT_FAILURE);
  }
  return 0;
}

int decode(moodycamel::ConcurrentQueue<SpikeRaster_t>& q)
{
  while (true) {
    SpikeRaster_t found;
    bool result = q.try_dequeue(found);
    if (result) {
      std::cout << std::hex << found.id << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return 0;
}

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
  auto q = moodycamel::ConcurrentQueue<SpikeRaster_t>();

  std::thread receiver(initialize, ip, port, std::ref(q));     // spawn new thread that calls foo()
  std::thread decoder(decode, std::ref(q));  // spawn new thread that calls bar(0)

  std::cout << "Now executing concurrently...\n" << std::endl;

  // synchronize threads:
  receiver.join();                // pauses until first finishes
  decoder.join();               // pauses until second finishes

  //  initialize learner process

  return 0;
}
