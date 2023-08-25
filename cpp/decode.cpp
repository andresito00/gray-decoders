#include <iostream>
#include <csignal>
#include <chrono>
#include <memory>
#include <unistd.h>
#include <safer_thread.h>
#include <raster.h>
#include <receiver.h>
#include <tcp.h>
#include <concurrentqueue.h>
#include <tclap/CmdLine.h>

using SpikeRaster64 = raster::SpikeRaster64;
using RasterQueue = moodycamel::ConcurrentQueue<SpikeRaster64>;
using CmdLine = TCLAP::CmdLine;
using StringArg = TCLAP::ValueArg<std::string>;
using UShortArg = TCLAP::ValueArg<uint16_t>;
using ArgException = TCLAP::ArgException;
using ReceiverStatus = receiver::ReceiverStatus;
using LinuxTCPReceiver = receiver::Receiver<LinuxTCPCore, RasterQueue>;
using SafeThread = sthread::SaferThread;

void receive(RasterQueue& q)
{
  LinuxTCPReceiver *receiver = new LinuxTCPReceiver(4096);
  if (receiver::ReceiverStatus::kOkay == receiver->get_status()) {
    receiver->receive(q);
  } else {
    delete receiver;
    exit(EXIT_FAILURE);
  }
}

void decode(RasterQueue& q)
{
  SpikeRaster64 found;
  size_t count = 0;
  while (true) {
    while (!q.try_dequeue(found)) {
    // bug in clang-tidy-12 spaceship operator parsing:
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // workaround:
      std::this_thread::sleep_until(
        std::chrono::system_clock::now() +
        std::chrono::microseconds(256)
      );
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

  auto raster_queue = RasterQueue();
  auto decodes = SafeThread(std::thread(decode, std::ref(raster_queue)), sthread::Action::kJoin);
  auto receives = SafeThread(std::thread(receive, std::ref(raster_queue)), sthread::Action::kJoin);

  std::cout << "Now executing concurrently...\n" << std::endl;

  // Todo: write a logging thread instead of using stdout.

  return 0;
}
