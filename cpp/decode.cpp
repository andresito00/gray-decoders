#include <iostream>
#include <csignal>
#include <memory>
#include <unistd.h>
#include <safer_thread.h>
#include <raster.h>
#include <receiver.h>
#include <tcp.h>
#include <log.h>
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

static constexpr size_t kDefaultRxSize = 4096LU;

void receive(RasterQueue &q)
{
  LinuxTCPReceiver *receiver = new LinuxTCPReceiver(kDefaultRxSize);
  if (receiver::ReceiverStatus::kOkay == receiver->get_status()) {
    receiver->receive(q);
  } else {
    delete receiver;
    exit(EXIT_FAILURE);
  }
}

void decode(RasterQueue &q)
{
  SpikeRaster64 found;
  size_t count = 0;
  while (true) {
    q.wait_dequeue(found);
    LOG("# " + std::to_string(++count) + " ID: " + std::to_string(found.id));
    for (auto r : found.raster) {
      LOG("\t" + std::to_string(r));
    }
  }
}

int main(int argc, char *argv[])
{
  std::string ip;
  uint16_t port;
  try {
    CmdLine cmd("CLI interface to launch the decoder", ' ', "0.0");
    StringArg arg_ip("i", "ip", "IP address to bind", true, "", "string");
    UShortArg arg_port("p", "port", "Port to listen on", true, 8808, "int");

    cmd.add(arg_ip);
    cmd.add(arg_port);

    cmd.parse(argc, argv);

    ip = arg_ip.getValue();
    port = arg_port.getValue();

    LOG("Binding to " + ip + ": " + std::to_string(port));
  } catch (ArgException &e) {
    LOG_ERROR("error: " + e.error() + " for arg " + e.argId());
  }

  auto raster_queue = RasterQueue();
  auto decodes = SafeThread(std::thread(decode, std::ref(raster_queue)),
                            sthread::Action::kJoin);
  auto receives = SafeThread(std::thread(receive, std::ref(raster_queue)),
                             sthread::Action::kJoin);

  LOG("Now executing concurrently...");
  // Todo: write a logging thread instead of using stdout.
  std::cout << std::flush;
  return 0;
}
