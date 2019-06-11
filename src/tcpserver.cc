#include <functional>
#include <cassert>

#include "tcpserver.hh"
#include "tcpconn.hh"
#include "acceptor.hh"
#include "eventloop.hh"
#include "eventloop_pool.hh"
#include "inetaddr.hh"
#include "util/log.hh"

namespace axs {

using std::placeholders::_1;
using std::placeholders::_2;

TcpServer::TcpServer(EventLoop& loop, const InetAddr& addr)
    : loop_{loop},
      loop_poolp_{std::make_unique<EventLoopPool>(loop_)},
      acceptorp_{std::make_unique<Acceptor>(loop, addr)} {
    LOG_INFO << "TcpServer(" << this << ") created";
    acceptorp_->SetNewConnCallback(
                    std::bind(&TcpServer::HandleNewConn, this, _1, _2));
}

TcpServer::~TcpServer() {
    LOG_INFO << "TcpServer(" << this << ") destructs";
    for (auto& item : conns_) {
        TcpConnPtr connp = item.second;
        connp->OwnerLoop().RunInLoop([=]() { connp->OnDisconnected(); });
    }
}

void TcpServer::SetThreadNum(int n) {
    loop_poolp_->SetThreadNum(n);
}

void TcpServer::Start() {
    LOG_INFO << "TcpServer(" << this << ") starts";
    loop_poolp_->Start();
    // TODO: If the server destructs before the execution of this task,
    // acceptorp_ will be invalid.
    loop_.RunInLoop([&]() { acceptorp_->Listen(); });
}

void TcpServer::HandleNewConn(int sk, const InetAddr& peer_addr) {
    LOG_DEBUG << "TcpServer(" << this << ") handle new connection comes from "
              << peer_addr.Ip() << ":" << peer_addr.Port();
    loop_.AssertInLoopThread();
    EventLoop& conn_loop = loop_poolp_->GetNextLoop();
    TcpConnPtr connp = std::make_shared<TcpConn>(conn_loop, sk, peer_addr);
    connp->SetConnectedCallback(connnected_cb_);
    connp->SetDisconnectedCallback(disconnected_cb_);
    connp->SetRecvCallback(recv_cb_);
    connp->SetWriteCompCallback(write_comp_cb_);
    connp->SetCloseCallback(std::bind(&TcpServer::HandleConnClose, this, _1));
    conns_.emplace(sk, connp);
    conn_loop.RunInLoop([=]() { connp->OnConnected(); });
}

void TcpServer::HandleConnClose(TcpConnPtr connp) {
    // It has to be captured by value.
    loop_.RunInLoop([=](){ HandleConnCloseInLoop(connp); });
}

void TcpServer::HandleConnCloseInLoop(TcpConnPtr connp) {
    LOG_DEBUG << "TcpServer(" << this << ") handle TcpConn(" << connp.get()
              << ") closing";
    loop_.AssertInLoopThread();
    std::size_t erased = conns_.erase(connp->SocketFd());
    assert(erased == 1);
    // Note that if the user does not owe a copy of this TcpConnPtr now, this
    // will be the last shared_ptr to this TcpConn object, which means this
    // TcpConn object will destructs in the end of the current function while
    // we are still inside the member function of its member object PollFd.
    // So we have to extend its life by storing a copy of its pointer into the
    // task queue of its owner loop. And note that it has to be QueueInLoop()
    // because RunInLoop() here may be equivalent to an immediate call.
    EventLoop& conn_loop = connp->OwnerLoop();
    // It has to be captured by value.
    conn_loop.QueueInLoop([=]() { connp->OnDisconnected(); });
}

}
