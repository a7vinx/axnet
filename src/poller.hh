#ifndef _AXS_POLLER_HH_
#define _AXS_POLLER_HH_

#include <string>
#include <map>
#include <vector>
#include <cstdlib>
#include <boost/core/noncopyable.hpp>
#include <unistd.h>
#include <sys/epoll.h>

namespace axs {

// Forward declaration.
class EventLoop;
class PollFd;

class Poller : private boost::noncopyable {
public:
    Poller(EventLoop& loop, std::size_t max_events_once = 16);
    ~Poller() {
        ::close(epfd_);
    }
    std::vector<PollFd*> Poll(int timeout);
    void UpdateFd(PollFd* fd);
    void RemoveFd(PollFd* fd);

private:
    static std::string EpollOpToStr(int op);
    void EpollCtl(int op, PollFd* fdp);

    EventLoop& loop_;
    int epfd_;
    std::vector<struct epoll_event> events_;
    std::map<int, PollFd*> fd_map_{};
};

}
#endif
