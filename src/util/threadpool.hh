#ifndef _AXS_THREADPOOL_HH_
#define _AXS_THREADPOOL_HH_

#include <functional>
#include <deque>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <boost/core/noncopyable.hpp>

namespace axs {

class ThreadPool : private boost::noncopyable {
public:
    using Functor = std::function<void()>;

    ThreadPool() = default;
    ~ThreadPool();

    void SetThreadNum(int n) { thread_num_ = n; }
    void SetThreadInitCallback(Functor f) { thread_init_cb_ = f; }
    bool IsRunning() const { return running_; }
    void Start();
    void Stop();
    void AddTask(Functor f);

private:
    Functor GetTask();
    void ThreadFunc();

    // Normal bool may cause worker thread to wait for tasks, not
    // responding to the stop command.
    std::atomic_bool running_{false};
    int thread_num_{1};
    mutable std::mutex tasks_mutex_{};
    mutable std::condition_variable not_empty_{};
    std::deque<Functor> tasks_{};
    std::vector<std::thread> pool_{};
    Functor thread_init_cb_{};
};

}
#endif
