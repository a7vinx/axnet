#include <algorithm>
#include <cassert>

#include "eventloop.hh"
#include "poller.hh"
#include "pollfd.hh"
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
    while (!quit_) {
        ready_fds_ = pollerp_->Poll(poll_timeout_);
        HandleEvents();
        ready_fds_.clear();
    }
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
    // Allow the fd to remove itself and other unready fds.
    if (event_handling_) {
        assert(fdp == cur_handling_fd_ ||
               std::find(ready_fds_.cbegin(), ready_fds_.cend(), fdp)
                   == ready_fds_.cend());
    }
    pollerp_->RemoveFd(fdp);
}

void EventLoop::HandleEvents() {
    event_handling_ = true;
    for (const auto& fdp : ready_fds_) {
        cur_handling_fd_ = fdp;
        fdp->HandleEvent();
    }
    cur_handling_fd_ = nullptr;
    event_handling_ = false;
}

}
