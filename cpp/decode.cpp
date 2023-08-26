#include <iostream>
#include <csignal>
#include <chrono>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <safer_thread.h>
#include <raster.h>
#include <receiver.h>
#include <tcp.h>
#include <blockingconcurrentqueue.h>
#include <tclap/CmdLine.h>

using SpikeRaster64 = raster::SpikeRaster64;
using RasterQueue = moodycamel::BlockingConcurrentQueue<SpikeRaster64>;
using CmdLine = TCLAP::CmdLine;
using StringArg = TCLAP::ValueArg<std::string>;
using UShortArg = TCLAP::ValueArg<uint16_t>;
using ArgException = TCLAP::ArgException;
using ReceiverStatus = receiver::ReceiverStatus;
using SafeThread = sthread::SaferThread;
using LinuxTCPReceiver = receiver::Receiver<LinuxTCPCore, RasterQueue>;

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
    q.wait_dequeue(found);
    std::cout << "# " << ++count << " ID: " << std::hex << found.id << '\n';
    for (auto r: found.raster) {
      std::cout << "\t" << std::dec << r << '\n';
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

    std::cout << "Binding to " << ip << ": " << port << '\n';

  } catch (ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId()
              << '\n';
  }

  auto raster_queue = RasterQueue();
  auto decodes = SafeThread(std::thread(decode, std::ref(raster_queue)), sthread::Action::kJoin);
  auto receives = SafeThread(std::thread(receive, std::ref(raster_queue)), sthread::Action::kJoin);

  std::cout << "Now executing concurrently...\n" << '\n';
  // Todo: write a logging thread instead of using stdout.
  std::cout << std::flush;
  return 0;
}
