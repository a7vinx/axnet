#ifndef _AXS_EVENTLOOP_HH_
#define _AXS_EVENTLOOP_HH_

#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <boost/core/noncopyable.hpp>

namespace axs {

// Forward declaration.
class PollFd;
class Poller;

class EventLoop : private boost::noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // Trivial getters/setters.
    void SetPollTimeOut(int timeout) { poll_timeout_ = timeout; }
    void Quit() { quit_ = true; }

    // Non-trivial member functions.
    void Loop();
    void RunInLoop(Functor f);
    void QueueInLoop(Functor f);

    // Helpers.
    bool IsInLoopThread() const {
        return std::this_thread::get_id() == thread_id_; }
    void AssertInLoopThread();
    void UpdatePollFd(PollFd* fdp);
    void RemovePollFd(PollFd* fdp);

private:
    void HandleEvents();
    void DoPendingTasks();
    void Wakeup();
    void HandleWakeupFdReading();

    std::thread::id thread_id_;
    // Use unique_ptr to reduce header dependencies.
    std::unique_ptr<Poller> pollerp_;
    std::vector<PollFd*> ready_fds_{};
    PollFd* cur_handling_fd_{nullptr};
    int poll_timeout_{10000};
    // Actually whether to use atomic bool does not affect the correctness.
    std::atomic_bool quit_{false};
    std::atomic_bool event_handling_{false};
    // Variables for pending tasks.
    mutable std::mutex pending_tasks_mutex_{};
    std::vector<Functor> pending_tasks_{};
    std::unique_ptr<PollFd> wakeup_fdp_;
    std::atomic_bool doing_pending_tasks_{false};
};

}
#endif
