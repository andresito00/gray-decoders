#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <unistd.h>
#include <tclap/CmdLine.h>
#include <util.h>
#include <receiver_tcp.h>
#include <concurrentqueue.h>

using namespace moodycamel;
using namespace TCLAP;

void receive(std::string ip, uint16_t port, ConcurrentQueue<struct SpikeRaster> &q)
{
  ReceiverTcp *receiver = new ReceiverTcp(ip, port, 4096);
  receiver->initialize();
  ReceiverStatus_e status = receiver->receive(q);
  // thread lives in receive
  if (status == RECEIVER_STATUS_ERROR) {
    exit(EXIT_FAILURE);
  }
}

void decode(ConcurrentQueue<struct SpikeRaster> &q)
{
  size_t count = 0;
  while (true) {
    struct SpikeRaster found;
    bool result = q.try_dequeue(found);
    if (result) {
      std::cout << "# " << ++count << " ID: " << std::hex << found.id << std::endl;
    }
    // bug in clang-tidy-12 spaceship operator parsing...
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::this_thread::sleep_until(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(1)
    );
  }
}

int main(int argc, char *argv[])
{
  std::string ip;
  uint16_t port;
  try {
    CmdLine cmd("CLI interface to launch the decoder", ' ', "0.0");
    ValueArg<std::string> arg_ip("i", "ip", "IP address to bind to",
                                        true, "", "string");
    ValueArg<uint16_t> arg_port("p", "port", "Port to listen on", true,
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
  auto q = ConcurrentQueue<struct SpikeRaster>();

  auto decodes = std::thread(decode, std::ref(q));  // spawn new thread that calls bar(0)
  auto receives = std::thread(receive, ip, port, std::ref(q));

  std::cout << "Now executing concurrently...\n" << std::endl;

  // synchronize threads:
  decodes.join();   // pauses until second finishes
  receives.join();  // pauses until first finishes


  //  initialize learner process

  return 0;
}
