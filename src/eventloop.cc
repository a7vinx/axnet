#include "eventloop.hh"
#include "poller.hh"
#include "util/log.hh"

namespace axs {

namespace {
    thread_local EventLoop* tlocal_loop = nullptr;
} // unnamed namespace

EventLoop::EventLoop()
    : thread_id_{std::this_thread::get_id()},
      pollerp_{std::make_unique<Poller>(*this)} {
    LOG_INFO << "EventLoop(" << this << ") Created";
    if (tlocal_loop != nullptr) {
        LOG_FATAL << "Thread(" << thread_id_ << ") already has an EventLoop("
                  << tlocal_loop << ")";
    } else {
        tlocal_loop = this;
    }
}

void EventLoop::Loop() {
}

void EventLoop::AssertInLoopThread() {
    if (!IsInLoopThread())
        LOG_FATAL << "EventLoop::AssertInLoopThread() Failed - "
                  << "The thread id of EventLoop: " << thread_id_
                  << " Current thread id: " << std::this_thread::get_id();
}

void EventLoop::UpdatePollFd(PollFd* fdp) {
    AssertInLoopThread();
    pollerp_->UpdateFd(fdp);
}

void EventLoop::RemovePollFd(PollFd* fdp) {
    AssertInLoopThread();
    pollerp_->RemoveFd(fdp);
}

}
