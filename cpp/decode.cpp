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
#include <concurrentqueue.h>
#include <tclap/CmdLine.h>

using SpikeRaster64 = raster::SpikeRaster64;
using CmdLine = TCLAP::CmdLine;
using StringArg = TCLAP::ValueArg<std::string>;
using UShortArg = TCLAP::ValueArg<uint16_t>;
using ArgException = TCLAP::ArgException;
using ReceiverStatus = receiver::ReceiverStatus;
using SafeThread = sthread::SaferThread;

template<typename T>
class SharedQueue {
public:
  template<typename... Args>
  void enqueue_bulk(Args&&... args) {
    raster_queue_.enqueue_bulk(std::forward<Args>(args)...);
    condition_.notify_one();
  }

  template<typename... Args>
  bool wait_and_dequeue(Args&&... args) {
    condition_.wait(lk, [this]{return this->raster_queue_.size_approx() > 0;});
    auto result = raster_queue_.try_dequeue(std::forward<Args>(args)...);
    return result;
  }

  bool size_approx() const {
    return raster_queue_.size_approx() == 0;
  }

private:
  moodycamel::ConcurrentQueue<T> raster_queue_;
  std::condition_variable condition_;
};

using LinuxTCPReceiver = receiver::Receiver<LinuxTCPCore, SharedQueue<SpikeRaster64>>;

void receive(SharedQueue<SpikeRaster64>& q)
{
  LinuxTCPReceiver *receiver = new LinuxTCPReceiver(4096);
  if (receiver::ReceiverStatus::kOkay == receiver->get_status()) {
    receiver->receive(q);
  } else {
    delete receiver;
    exit(EXIT_FAILURE);
  }
}

void decode(SharedQueue<SpikeRaster64>& q)
{
  SpikeRaster64 found;
  size_t count = 0;
  while (true) {
    if (q.wait_and_dequeue(found)) {
      std::cout << "# " << ++count << " ID: " << std::hex << found.id << '\n';
      for (auto r: found.raster) {
        std::cout << "\t" << std::dec << r << '\n';
      }
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

  auto raster_queue = SharedQueue<SpikeRaster64>();
  auto decodes = SafeThread(std::thread(decode, std::ref(raster_queue)), sthread::Action::kJoin);
  auto receives = SafeThread(std::thread(receive, std::ref(raster_queue)), sthread::Action::kJoin);

  std::cout << "Now executing concurrently...\n" << '\n';
  // Todo: write a logging thread instead of using stdout.
  std::cout << std::flush;
  return 0;
}
