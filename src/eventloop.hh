#ifndef _AXS_EVENTLOOP_HH_
#define _AXS_EVENTLOOP_HH_

#include <vector>
#include <thread>
#include <memory>
#include <boost/core/noncopyable.hpp>

namespace axs {

// Forward declaration.
class PollFd;
class Poller;

class EventLoop : private boost::noncopyable {
public:
    EventLoop();

    // Trivial getters/setters.
    void SetPollTimeOut(int timeout) { poll_timeout_ = timeout; }
    void Quit() { quit_ = true; }

    // Non-trivial member functions.
    void Loop();
    bool IsInLoopThread() const {
        return std::this_thread::get_id() == thread_id_; }
    void AssertInLoopThread();
    void UpdatePollFd(PollFd* fdp);
    void RemovePollFd(PollFd* fdp);

private:
    void HandleEvents();

    std::thread::id thread_id_;
    std::unique_ptr<Poller> pollerp_;
    std::vector<PollFd*> ready_fds_{};
    PollFd* cur_handling_fd_{nullptr};
    int poll_timeout_{10000};
    bool quit_{false};
    bool event_handling_{false};
};

}
#endif
