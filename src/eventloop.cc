#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <boost/format.hpp>
#include <sys/eventfd.h>
#include <unistd.h>

#include "eventloop.hh"
#include "poller.hh"
#include "pollfd.hh"
#include "util/log.hh"

namespace axn {

namespace {

thread_local EventLoop* tlocal_loop = nullptr;

// Create an event fd.
int EventFd() {
    int fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (fd < 0)
        LOG_FATAL << "Creating event fd failed with errno " << errno
                  << " : " << StrError(errno);
    return fd;
}

} // unnamed namespace

EventLoop::EventLoop()
    : thread_id_{std::this_thread::get_id()},
      pollerp_{std::make_unique<Poller>(*this)},
      wakeup_fdp_{std::make_unique<PollFd>(*this, EventFd())} {
    LOG_INFO << "EventLoop(" << this << ") Created";
    if (tlocal_loop != nullptr) {
        LOG_FATAL << "Thread(" << boost::format("%#018x") % thread_id_
                  << ") already has an EventLoop(" << tlocal_loop << ")";
    } else {
        tlocal_loop = this;
    }
    wakeup_fdp_->SetReadCallback([&]() { return HandleWakeupFdReading(); });
    wakeup_fdp_->EnableReading();
}

EventLoop::~EventLoop() {
    LOG_INFO << "EventLoop(" << this << ") Destructs";
    wakeup_fdp_->RemoveFromLoop();
    tlocal_loop = nullptr;
}

void EventLoop::Loop() {
    while (!quit_) {
        ready_fds_ = pollerp_->Poll(poll_timeout_);
        HandleEvents();
        ready_fds_.clear();
        DoPendingTasks();
    }
}

void EventLoop::AssertInLoopThread() {
    if (!IsInLoopThread())
        LOG_FATAL << "EventLoop::AssertInLoopThread() Failed - "
                  << "The thread id of EventLoop: "
                  << boost::format("%#018x") % thread_id_
                  << " Current thread id: "
                  << boost::format("%#018x") % std::this_thread::get_id();
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

void EventLoop::RunInLoop(Functor f) {
    if (IsInLoopThread()) {
        f();
    } else {
        QueueInLoop(std::move(f));
    }
}

void EventLoop::QueueInLoop(Functor f) {
    {
        std::lock_guard<std::mutex> lock{pending_tasks_mutex_};
        pending_tasks_.emplace_back(std::move(f));
    }
    // We can't directly append it to the vector of pending functors so it
    // should be done in the next loop.
    if (!IsInLoopThread() || doing_pending_tasks_)
        Wakeup();
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

void EventLoop::DoPendingTasks() {
    doing_pending_tasks_ = true;
    std::vector<Functor> workload{};
    {
        std::lock_guard<std::mutex> lock{pending_tasks_mutex_};
        workload.swap(pending_tasks_);
    }
    for (const auto& f : workload)
        f();
    doing_pending_tasks_ = false;
}

void EventLoop::Wakeup() {
    std::uint64_t one = 1;
    int n = ::write(wakeup_fdp_->Fd(), &one, sizeof(one));
    // Return value seems enough.
    if (n < sizeof(one))
        LOG_ERROR << "Unexpected return value " << n
                  << " when writing to waking up fd";
}

void EventLoop::HandleWakeupFdReading() {
    std::uint64_t one = 0;
    int n = ::read(wakeup_fdp_->Fd(), &one, sizeof(one));
    if (n < sizeof(one))
        LOG_ERROR << "Unexpected return value " << n
                  << " when reading from waking up fd";
}

}
