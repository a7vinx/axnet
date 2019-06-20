#ifndef _AXS_POLLFD_HH_
#define _AXS_POLLFD_HH_

#include <string>
#include <functional>
#include <boost/core/noncopyable.hpp>
#include <sys/epoll.h>

namespace axs {

// Forward declaration.
class EventLoop;

// Responsible for the management of the life cycle of the contained fd.
class PollFd : private boost::noncopyable {
public:
    using EventCallback = std::function<void()>;

    PollFd(EventLoop& loop, int fd) : loop_{loop}, fd_{fd} {}
    ~PollFd();

    // Trivial getters and setters.
    int Fd() const { return fd_; }
    int Events() const { return events_; }
    void SetRevents(int revents) { revents_ = revents; }
    std::string EventsToStr() const { return EventsToStr(events_); }
    std::string ReventsToStr() const { return EventsToStr(revents_); }

    // Event switchers.
    void EnableReading() { events_ |= kETRead; NotifyLoop(); }
    void DisableReading() { events_ &= ~kETRead; NotifyLoop(); }
    void EnableWriting() { events_ |= kETWrite; NotifyLoop(); }
    void DisableWriting() { events_ &= ~kETWrite; NotifyLoop(); }
    void DisableRw() { events_ = 0; NotifyLoop(); }
    bool IsReading() const { return events_ & kETRead; }
    bool IsWriting() const { return events_ & kETWrite; }
    bool CareNoEvent() const { return events_ == 0; }

    // Callback setters.
    void SetReadCallback(EventCallback cb) { read_cb_ = cb; }
    void SetWriteCallback(EventCallback cb) { write_cb_ = cb; }
    void SetErrorCallback(EventCallback cb) { err_cb_ = cb; }

    // Non-trivial member functions.
    void HandleEvent();
    // Split it from destructor because we need to ensure that the poller
    // is still valid which can't be guaranteed during the destruction of
    // owner loop.
    void RemoveFromLoop();
    // Detach the life cycle of fd.
    int DetachFd();

private:
    enum EventType {
        kETRead = EPOLLIN | EPOLLPRI,
        kETWrite = EPOLLOUT
    };

    void NotifyLoop();
    static std::string EventsToStr(int events);

    EventLoop& loop_;
    int fd_;
    int events_{0};
    int revents_{0};
    EventCallback read_cb_{};
    EventCallback write_cb_{};
    EventCallback err_cb_{};
    bool event_handling_{false};
    bool is_in_loop_{false};
    bool fd_detached_{false};
};

}
#endif
