#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <unistd.h>
#include <util.h>
#include <receiver.h>
#include <tcp.h>
#include <concurrentqueue.h>
#include <tclap/CmdLine.h>

using namespace moodycamel;
using namespace TCLAP;

template<typename Q>
void receive(Q& q)
{
  Receiver<LinuxTCPCore, Q> *receiver = new Receiver<LinuxTCPCore, Q>(4096);
  if (ReceiverStatus::kOkay == receiver->get_status()) {
    receiver->Receive(q);
  } else {
    delete receiver;
    exit(EXIT_FAILURE);
  }
}

template<typename Q>
void decode(Q& q)
{
  size_t count = 0;
  while (true) {
    struct SpikeRaster found;
    while(!q.try_dequeue(found)); // block this thread here
    std::cout << "# " << ++count << " ID: " << std::hex << found.id << std::endl;
    // bug in clang-tidy-12 spaceship operator parsing:
    //  std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // workaround:
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
  auto decodes = std::thread(decode<ConcurrentQueue<struct SpikeRaster>>, std::ref(q));
  auto receives = std::thread(receive<ConcurrentQueue<struct SpikeRaster>>, std::ref(q));

  std::cout << "Now executing concurrently...\n" << std::endl;

  // todo: write a logging thread instead of using stdout.
  // this will ensure we're not trapping into the OS to provide debug output in the middle
  // of performance critical compute.

  // synchronize threads:
  // decodes.join();   // pauses until second finishes
  // receives.join();  // pauses until first finishes

  //  initialize learner process

  return 0;
}
