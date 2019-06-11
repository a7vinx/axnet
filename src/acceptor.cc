#include <cassert>

#include "acceptor.hh"
#include "eventloop.hh"
#include "util/log.hh"

namespace axs {

Acceptor::Acceptor(EventLoop& loop, const InetAddr& addr)
    : loop_{loop},
      listen_addr_{addr},
      poll_fd_{loop_, NonBlockTcpSocket()},
      sk_op_{poll_fd_.Fd()} {
    sk_op_.SetReuseAddr(true);
    sk_op_.SetReusePort(true);
    sk_op_.Bind(addr);
    poll_fd_.SetReadCallback([&](){ HandleAccept(); });
}

Acceptor::~Acceptor() {
    loop_.AssertInLoopThread();
    poll_fd_.RemoveFromLoop();
}

void Acceptor::Listen() {
    loop_.AssertInLoopThread();
    poll_fd_.EnableReading();
    sk_op_.Listen();
}

void Acceptor::HandleAccept() {
    std::pair<int, InetAddr> conn_pair = sk_op_.Accept();
    if (conn_pair.first < 0) {
        // TODO: The idle file trick seems not perfect so maybe a soft limit
        // should be used?
    } else {
        assert(new_conn_cb_);
        new_conn_cb_(conn_pair.first, conn_pair.second);
    }
}

}
