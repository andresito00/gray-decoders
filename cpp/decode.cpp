#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <memory>
#include <unistd.h>
#include <raster.h>
#include <receiver.h>
#include <tcp.h>
#include <concurrentqueue.h>
#include <tclap/CmdLine.h>

using SpikeRaster64 = raster::SpikeRaster64;
using RasterQueue = moodycamel::ConcurrentQueue<raster::SpikeRaster64>;
using CmdLine = TCLAP::CmdLine;
using StringArg = TCLAP::ValueArg<std::string>;
using UShortArg = TCLAP::ValueArg<uint16_t>;
using ArgException = TCLAP::ArgException;

template<typename Q>
void receive(Q& q)
{
  Receiver<LinuxTCPCore, Q> *receiver = new Receiver<LinuxTCPCore, Q>(4096);
  if (ReceiverStatus::kOkay == receiver->get_status()) {
    receiver->receive(q);
  } else {
    delete receiver;
    exit(EXIT_FAILURE);
  }
}

template<typename Q>
void decode(Q& q)
{
  SpikeRaster64 found;
  size_t count = 0;
  while (true) {
    while (!q.try_dequeue(found)) {
    // bug in clang-tidy-12 spaceship operator parsing:
    //  std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // workaround:
      std::this_thread::sleep_until(
        std::chrono::system_clock::now() +
        std::chrono::microseconds(256)
      ); // block this thread here
    }
    std::cout << "# " << ++count << " ID: " << std::hex << found.id << std::endl;
    for (auto r: found.raster) {
      std::cout << "\t" << std::dec << r << std::endl;
    }
  }
}

int main(int argc, char *argv[])
{
  std::string ip;
  uint16_t port;
  try {
    CmdLine cmd("CLI interface to launch the decoder", ' ', "0.0");
    StringArg arg_ip("i", "ip", "IP address to bind to",
                                        true, "", "string");
    UShortArg arg_port("p", "port", "Port to listen on", true,
                                       8080, "int");

    cmd.add(arg_ip);
    cmd.add(arg_port);

    cmd.parse(argc, argv);

    ip = arg_ip.getValue();
    port = arg_port.getValue();

    std::cout << "Binding to " << ip << ": " << port << std::endl;

  } catch (ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId()
              << std::endl;
  }

  // main routine should eventually be:
  //  accessible runtime configuration given args/initialization input
  //  initialize receiver process
  auto raster_queue = RasterQueue();
  auto decodes = std::thread(decode<RasterQueue>, std::ref(raster_queue));
  auto receives = std::thread(receive<RasterQueue>, std::ref(raster_queue));

  std::cout << "Now executing concurrently...\n" << std::endl;

  // todo: write a logging thread instead of using stdout.
  // this will ensure we're not trapping into the OS to provide debug output in the middle
  // of performance critical compute.

  // synchronize threads:
  decodes.join();   // pauses until second finishes
  receives.join();  // pauses until first finishes

  //  initialize learner process

  return 0;
}
