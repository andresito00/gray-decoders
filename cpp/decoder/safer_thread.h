#ifndef DECODER_RAII_THREAD_H_
#define DECODER_RAII_THREAD_H_

#include <thread>

namespace sthread {

enum class Action {
  kJoin,
  kDetach
};

class SaferThread {
public:
  SaferThread(std::thread&& thread, Action action):
    thread_{std::move(thread)}, action_{action} {}

  ~SaferThread()
  {
    if (!thread_.joinable()) {
      return;
    }
    if (action_ == Action::kJoin) {
      thread_.join();
    } else {
      thread_.detach();
    }
  }

  std::thread& get() noexcept
  {
    return thread_;
  }

private:
  std::thread thread_;
  Action action_;
};

};


#endif // DECODER_RAII_THREAD_H_