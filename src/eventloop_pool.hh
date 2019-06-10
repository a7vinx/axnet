#ifndef _AXS_EVENTLOOP_POOL_HH_
#define _AXS_EVENTLOOP_POOL_HH_

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <boost/core/noncopyable.hpp>

namespace axs {

// Forward declaration.
class EventLoop;

class EventLoopPool : private boost::noncopyable {
public:
    EventLoopPool(EventLoop& loop) : loop_{loop} {}
    ~EventLoopPool();

    void SetThreadNum(int n);
    bool IsRunning() const { return running_; }
    void Start();
    void Stop();
    // Round-robin.
    EventLoop& GetNextLoop();
    std::vector<EventLoop*> GetAllLoop() const { return loop_pool_; }

private:
    void LoopThreadFunc();

    EventLoop& loop_;
    // Normal bool may cause Stop() to be called twice.
    std::atomic_bool running_{false};
    int thread_num_{0};
    int next_id_{-1};
    std::vector<std::thread> thread_pool_{};
    mutable std::mutex loop_pool_mutex_{};
    mutable std::condition_variable loop_pool_ready_{};
    std::vector<EventLoop*> loop_pool_{};
};

}
#endif
