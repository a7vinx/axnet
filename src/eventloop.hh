#ifndef _AXS_EVENTLOOP_HH_
#define _AXS_EVENTLOOP_HH_

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
    void Loop();
    bool IsInLoopThread() const {
        return std::this_thread::get_id() == thread_id_; }
    void AssertInLoopThread();
    void UpdatePollFd(PollFd* fdp);
    void RemovePollFd(PollFd* fdp);

private:
    std::thread::id thread_id_;
    std::unique_ptr<Poller> pollerp_;
};

}
#endif
