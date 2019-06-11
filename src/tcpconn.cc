#include <functional>
#include <cassert>

#include "tcpconn.hh"
#include "eventloop.hh"
#include "pollfd.hh"
#include "socketop.hh"
#include "util/log.hh"

namespace axs {

TcpConn::TcpConn(EventLoop& loop, int sk, const InetAddr& peer_addr)
    : loop_{loop},
      fdp_{std::make_unique<PollFd>(loop_, sk)},
      sk_opp_{std::make_unique<SocketOp>(sk)},
      local_addr_{sk_opp_->GetLocalAddr()},
      peer_addr_{peer_addr} {
    LOG_DEBUG << "TcpConn(" << this << ") created";
    sk_opp_->SetKeepAlive(true);
    fdp_->SetReadCallback([&]() { HandleRecv(); });
    fdp_->SetWriteCallback([&]() { HandleSend(); });
    fdp_->SetErrorCallback([&]() { HandleError(); });
}

TcpConn::~TcpConn() {
    LOG_DEBUG << "TcpConn(" << this << ") destructs";
    loop_.AssertInLoopThread();
    assert(state_ == ConnState::kDisconnected);
    fdp_->RemoveFromLoop();
}

int TcpConn::SocketFd() const {
    return fdp_->Fd();
}

void TcpConn::Send(const std::string& msg) {
    if (state_ != ConnState::kConnected) {
        LOG_WARN << "TcpConn(" << this << ") " << StateToStr()
                 << " , messages can not be sent";
    } else {
        // We have to store a shared_ptr to this connection object in this task
        // in case it destructs before the execution of this task.
        // TODO: Optimize string copy
        loop_.RunInLoop(
                  std::bind(&TcpConn::SendInLoop, shared_from_this(), msg));
    }
}

void TcpConn::ForceClose() {
    // TcpConn objects with the state of kConnecting is not exposed to the user.
    assert(state_ != ConnState::kConnecting);
    if (state_ != ConnState::kDisconnected) {
        // Same as what we do in Send(), store a shared_ptr to this.
        loop_.RunInLoop(
                  std::bind(&TcpConn::ForceCloseInLoop, shared_from_this()));
    }
    state_ = ConnState::kDisconnected;
}

void TcpConn::Shutdown() {
    // TcpConn objects with the state of kConnecting is not exposed to the user.
    assert(state_ != ConnState::kConnecting);
    // Use compare_exchange_strong() in case of race condition.
    ConnState connected_state{ConnState::kConnected};
    if (state_.compare_exchange_strong(connected_state,
                                       ConnState::kDisconnecting)) {
        // Same as what we do in Send(), store a shared_ptr to this.
        loop_.RunInLoop(std::bind(&TcpConn::ShutdownInLoop, shared_from_this()));
    }
}

void TcpConn::OnConnected() {
    LOG_INFO << "TcpConn(" << this << ") connected - Local address: "
             << local_addr_.Ip() << ":" << local_addr_.Port()
             << " Peer address: " << peer_addr_.Ip() << ":"
             << peer_addr_.Port();
    loop_.AssertInLoopThread();
    state_ = ConnState::kConnected;
    fdp_->EnableReading();
    if (connnected_cb_)
        connnected_cb_(shared_from_this());
}

void TcpConn::OnDisconnected() {
    LOG_INFO << "TcpConn(" << this << ") disconnected";
    loop_.AssertInLoopThread();
    // It may be called from the destructor of TcpServer.
    if (state_ != ConnState::kDisconnected) {
        state_ = ConnState::kDisconnected;
        fdp_->DisableRw();
    }
    if (disconnected_cb_)
        disconnected_cb_(shared_from_this());
}

void TcpConn::SendInLoop(const std::string& msg) {
    LOG_DEBUG << "TcpConn(" << this << ") sends messages - backlog: "
              << send_buf_.ReadableSize() << " bytes";
    loop_.AssertInLoopThread();
    // Due to the Gatekeeper Send(), if the state is kDisconnecting, this
    // message must come before Shutdown().
    if (state_ == ConnState::kDisconnected) {
        LOG_WARN << "TcpConn(" << this << ") disconnected, "
                 << "discard unsent buffer";
        return;
    }
    // If the sending buffer is empty, try to send directly.
    if (send_buf_.ReadableSize() == 0) {
        assert(!fdp_->IsWriting());
        int n = sk_opp_->Send(msg.c_str(), msg.size());
        if (n == msg.size()) {
            if (state_ == ConnState::kDisconnecting)
                sk_opp_->ShutdownWrite();
            if (write_comp_cb_)
                write_comp_cb_(shared_from_this());
            return;
        } else {
            n = n > 0 ? n : 0;
            send_buf_.Append(msg.c_str() + n, msg.size() - n);
        }
    } else {
        send_buf_.Append(msg);
    }
    if (!fdp_->IsWriting())
        fdp_->EnableWriting();
}

void TcpConn::ForceCloseInLoop() {
    loop_.AssertInLoopThread();
    assert(state_ != ConnState::kConnecting);
    // Check again because HandleClose() may have been triggered in the event
    // handling stage of the loop.
    if (state_ != ConnState::kDisconnected)
        HandleClose();
}

void TcpConn::ShutdownInLoop() {
    loop_.AssertInLoopThread();
    if (!fdp_->IsWriting()) {
        LOG_INFO << "TcpConn(" << this << ") is shut down for writing";
        sk_opp_->ShutdownWrite();
    }
}

void TcpConn::HandleRecv() {
    loop_.AssertInLoopThread();
    recv_buf_.ReserveWritable(65536);
    int n = sk_opp_->Recv(recv_buf_.WritableBegin(), recv_buf_.WritableSize());
    if (n > 0) {
        LOG_DEBUG << "TcpConn(" << this << ") received messages";
        assert(recv_cb_);
        recv_buf_.Written(n);
        recv_cb_(shared_from_this(), recv_buf_.RetrieveAll());
    } else if (n == 0) {
        HandleClose();
    } else {
        HandleError();
    }
}

void TcpConn::HandleSend() {
    LOG_DEBUG << "TcpConn(" << this << ") sends messages - backlog: "
              << send_buf_.ReadableSize() << " bytes";
    loop_.AssertInLoopThread();
    int n = sk_opp_->Send(send_buf_.ReadableBegin(), send_buf_.ReadableSize());
    n = n > 0 ? n : 0;
    send_buf_.Read(n);
    if (n == send_buf_.ReadableSize()) {
        fdp_->DisableWriting();
        if (state_ == ConnState::kDisconnecting)
            sk_opp_->ShutdownWrite();
        if (write_comp_cb_)
            write_comp_cb_(shared_from_this());
    }
}

void TcpConn::HandleClose() {
    LOG_INFO << "TcpConn(" << this << ") is closed";
    loop_.AssertInLoopThread();
    state_ = ConnState::kDisconnected;
    fdp_->DisableRw();
    assert(close_cb_);
    close_cb_(shared_from_this());
}

void TcpConn::HandleError() {
    loop_.AssertInLoopThread();
    int sock_errno = sk_opp_->GetError();
    if (sock_errno != 0)
        LOG_ERROR << "TcpConn(" << this << ") error occurred with errno "
                  << sock_errno << " : " << StrError(sock_errno);
}

std::string TcpConn::StateToStr() const {
    switch (state_) {
        case ConnState::kConnecting: return "connecting";
        case ConnState::kConnected: return "connected";
        case ConnState::kDisconnecting: return "disconnecting";
        case ConnState::kDisconnected: return "disconnected";
        default:
            assert(false);
    }
}

void DefaultRecvCallback(TcpConnPtr connp, std::string msg) {
    LOG_INFO << "TcpConn(" << connp.get() << ") received message: "  << msg;
}

}
