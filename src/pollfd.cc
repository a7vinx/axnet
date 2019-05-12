#include <unistd.h>

#include "pollfd.hh"
#include "eventloop.hh"
#include "util/log.hh"

namespace axs {

PollFd::~PollFd() {
    assert(!event_handling_);
    ::close(fd_);
}

void PollFd::HandleEvent() {
    event_handling_ = true;
    LOG_INFO << "Handle event: " << EventsToStr() << "on fd: " << fd_;
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
}

void PollFd::NotifyLoop() {
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
