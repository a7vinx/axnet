#include <unistd.h>
#include <cassert>

#include "pollfd.hh"
#include "eventloop.hh"
#include "util/log.hh"

namespace axn {

PollFd::~PollFd() {
    assert(!event_handling_);
    assert(!is_in_loop_);
    if (!fd_detached_)
        ::close(fd_);
}

void PollFd::HandleEvent() {
    event_handling_ = true;
    LOG_DEBUG << "Handle event: " << EventsToStr() << "on fd: " << fd_;
    if (revents_ & EPOLLERR) {
        if (err_cb_) err_cb_();
    }
    // Leave HUP events to reading callback because sometimes hanging up does
    // not trigger EPOLLHUP or EPOLLRDHUP, it only triggers normal EPOLLIN.
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP)) {
        if (read_cb_) read_cb_();
    }
    if (revents_ & EPOLLOUT) {
        if (write_cb_) write_cb_();
    }
    event_handling_ = false;
}

void PollFd::RemoveFromLoop() {
    loop_.AssertInLoopThread();
    is_in_loop_ = false;
    loop_.RemovePollFd(this);
}

int PollFd::DetachFd() {
    fd_detached_ = true;
    return fd_;
}

void PollFd::NotifyLoop() {
    loop_.AssertInLoopThread();
    is_in_loop_ = true;
    loop_.UpdatePollFd(this);
}

std::string PollFd::EventsToStr(int events) {
    std::string events_str_{};
    if (events & EPOLLIN) {
        events_str_ += "IN ";
    } else if (events & EPOLLOUT) {
        events_str_ += "OUT ";
    } else if (events & EPOLLRDHUP) {
        events_str_ += "RDHUP ";
    } else if (events & EPOLLPRI) {
        events_str_ += "PRI ";
    } else if (events & EPOLLERR) {
        events_str_ += "ERR ";
    } else if (events & EPOLLHUP) {
        events_str_ += "HUP ";
    }
    return events_str_;
}

}
