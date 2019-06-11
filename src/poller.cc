#include <cassert>
#include <cerrno>

#include "poller.hh"
#include "pollfd.hh"
#include "eventloop.hh"
#include "util/log.hh"

namespace axs {

Poller::Poller(EventLoop& loop, std::size_t max_events_once)
    : loop_{loop},
      epfd_{::epoll_create1(EPOLL_CLOEXEC)},
      events_{max_events_once} {
    if (epfd_ < 0)
        LOG_FATAL << "Creating epoll fd failed with errno " << errno
                  << ": " << StrError(errno);
}

std::vector<PollFd*> Poller::Poll(int timeout) {
    int ready_num = ::epoll_wait(
                        epfd_, &*events_.begin(), events_.size(), timeout);
    if (ready_num < 0 && errno != EINTR)
        LOG_FATAL << "epoll_wait() failed with errno " << errno
                  << ": " << StrError(errno);
    LOG_DEBUG << "Poll: " << ready_num << " events ready";
    // Prepare ready fds.
    std::vector<PollFd*> ready_fds{};
    for (int i = 0; i < ready_num; ++i) {
        PollFd* fdp = static_cast<PollFd*>(events_[i].data.ptr);
        fdp->SetRevents(events_[i].events);
        ready_fds.push_back(fdp);
    }
    return ready_fds;
}

void Poller::UpdateFd(PollFd* fdp) {
    loop_.AssertInLoopThread();
    auto iter = fd_map_.find(fdp->Fd());
    if (iter == fd_map_.cend()) {
        // A new one.
        fd_map_[fdp->Fd()] = fdp;
        EpollCtl(EPOLL_CTL_ADD, fdp);
    } else if (fdp->CareNoEvent()) {
        // TODO: Optimization
        fd_map_.erase(iter);
        EpollCtl(EPOLL_CTL_DEL, fdp);
    } else {
        EpollCtl(EPOLL_CTL_MOD, fdp);
    }
}

void Poller::RemoveFd(PollFd* fdp) {
    loop_.AssertInLoopThread();
    auto iter = fd_map_.find(fdp->Fd());
    if (iter != fd_map_.cend()) {
        fd_map_.erase(iter);
        EpollCtl(EPOLL_CTL_DEL, fdp);
    }
}

void Poller::EpollCtl(int op, PollFd* fdp) {
    LOG_DEBUG << "Poller: " << EpollOpToStr(op) << " fd " << fdp->Fd()
              << " with events: " << fdp->EventsToStr();
    struct epoll_event event;
    event.events = fdp->Events();
    event.data.ptr = fdp;
    int ctl_ret = ::epoll_ctl(epfd_, op, fdp->Fd(), &event);
    if (ctl_ret < 0)
        LOG_FATAL << "epoll_ctl() failed with errno " << errno
                  << ": " << StrError(errno);
}

std::string Poller::EpollOpToStr(int op) {
    switch (op) {
        case EPOLL_CTL_ADD: return "ADD";
        case EPOLL_CTL_MOD: return "MOD";
        case EPOLL_CTL_DEL: return "DEL";
        default:
            assert(false);
    }
}

}
