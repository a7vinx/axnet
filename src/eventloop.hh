#ifndef _AXS_EVENTLOOP_HH_
#define _AXS_EVENTLOOP_HH_

#include <thread>
#include <boost/core/noncopyable.hpp>

namespace axs {

class EventLoop : private boost::noncopyable {
public:
    EventLoop();
    void Loop();
    bool IsInLoopThread() const {
        return std::this_thread::get_id() == thread_id_; }
    void AssertInLoopThread();

private:
    std::thread::id thread_id_;
};

}
#endif
