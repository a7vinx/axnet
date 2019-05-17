#ifndef _AXS_ACCEPTOR_HH_
#define _AXS_ACCEPTOR_HH_

#include <functional>
#include <boost/core/noncopyable.hpp>

#include "pollfd.hh"
#include "inetaddr.hh"
#include "socketop.hh"

namespace axs {

// Forward declaration.
class EventLoop;

class Acceptor : private boost::noncopyable {
public:
    using NewConnCallback = std::function<void(int, const InetAddr&)>;

    Acceptor(EventLoop& loop, const InetAddr& addr);
    ~Acceptor();
    void SetNewConnCallback(NewConnCallback cb) { new_conn_cb_ = cb; }
    void Listen();

private:
    void HandleAccept();

    EventLoop& loop_;
    InetAddr listen_addr_;
    PollFd poll_fd_;
    SocketOp sk_op_;
    NewConnCallback new_conn_cb_{};
};

}
#endif
