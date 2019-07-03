#include <mutex>
#include <cassert>

#include "tcpclient.hh"
#include "eventloop.hh"
#include "tcpconn.hh"
#include "pollfd.hh"
#include "inetaddr.hh"
#include "socketop.hh"
#include "util/log.hh"

namespace axn {

using std::placeholders::_1;

class TcpClientImpl : public std::enable_shared_from_this<TcpClientImpl>,
                      private boost::noncopyable {
public:
    TcpClientImpl(EventLoop& loop, const InetAddr& dst_addr);
    ~TcpClientImpl();

    // Normal operations.
    void Connect();
    void Disconnect();
    void StopConnecting();

    // Callback setters.
    void SetConnectedCallback(ConnectedCallback cb) {
        connnected_cb_ = cb; }
    void SetDisconnectedCallback(DisconnectedCallback cb) {
        disconnected_cb_ = cb; }
    void SetRecvCallback(RecvCallback cb) { recv_cb_ = cb; }
    void SetWriteCompCallback(WriteCompCallback cb) {
        write_comp_cb_ = cb; }

    // Others.
    void EnableRetry() { retry_ = true; }
    void DisableRetry() { retry_ = false; }

private:
    enum class ClientState {
        kConnecting, kConnected, kDisconnected
    };

    void ConnectInLoop();
    void StopInLoop();
    void PrepareConnComp();
    void Retry();
    void HandleConnComp();
    void HandleConnError();
    void ClearConnFd(bool putoff_reset = true);
    void ConnEstablished(int conn_sk);
    // Closing handler set to connp_.
    void HandleConnClose(TcpConnPtr connp);

    EventLoop& loop_;
    InetAddr dst_addr_;
    std::unique_ptr<PollFd> conn_fdp_{};
    mutable std::mutex connp_mutex_{};
    TcpConnPtr connp_{};
    // Using atomic bool or not here does not affect correctness.
    std::atomic_bool connect_{false};
    // TODO: Retrying times.
    std::atomic_bool retry_{true};
    // No need to use the atomic version because all related reads and writes
    // are done in loop thread.
    ClientState state_{ClientState::kDisconnected};
    // Callbacks.
    ConnectedCallback connnected_cb_{};
    DisconnectedCallback disconnected_cb_{};
    RecvCallback recv_cb_{DefaultRecvCallback};
    WriteCompCallback write_comp_cb_{};
};

TcpClientImpl::TcpClientImpl(EventLoop& loop, const InetAddr& dst_addr)
    : loop_{loop}, dst_addr_{dst_addr} {
    LOG_INFO << "TcpClient(" << this << ") created";
}

TcpClientImpl::~TcpClientImpl() {
    LOG_INFO << "TcpClient(" << this << ") destructs";
    if (conn_fdp_) {
        // stl does not permit constructing std::function from a move-only type
        // so we use std::shared_ptr instead.
        std::shared_ptr<PollFd> conn_fdp{conn_fdp_.release()};
        loop_.RunInLoop([=]() { conn_fdp->RemoveFromLoop(); });
    }
    // Due to what we have done in ConnEstablished(), connp_ must be empty here.
    assert(!connp_);
    /*
    std::lock_guard<std::mutex> lock{connp_mutex_};
    if (connp_) {
        loop_.RunInLoop([=]() { connp_->OnDisconnected(); });
    }
    */
}

void TcpClientImpl::Connect() {
    LOG_DEBUG << "TcpClient(" << this << ") is ordered to connect to "
              << dst_addr_.Ip() << ":" << dst_addr_.Port();
    // Due to the existence of StopConnecting(), checking for repeated
    // connection here will cause problems.
    connect_ = true;
    loop_.RunInLoop(std::bind(&TcpClientImpl::ConnectInLoop, shared_from_this()));
}

void TcpClientImpl::Disconnect() {
    LOG_INFO << "TcpClient(" << this << ") disconnects";
    connect_ = false;
    std::lock_guard<std::mutex> lock{connp_mutex_};
    if (connp_) {
        connp_->Shutdown();
    }
}

void TcpClientImpl::StopConnecting() {
    LOG_DEBUG << "TcpClient(" << this << ") is ordered to stop";
    connect_ = false;
    // We need to proactively cancel the connection.
    loop_.RunInLoop(std::bind(&TcpClientImpl::StopInLoop, shared_from_this()));
}

void TcpClientImpl::ConnectInLoop() {
    loop_.AssertInLoopThread();
    // This connection may have been cancelled or repeated.
    if (!connect_ || state_ != ClientState::kDisconnected)
        return;
    LOG_INFO << "TcpClinet(" << this << ") starts to connect to "
             << dst_addr_.Ip() << ":" << dst_addr_.Port();
    state_ = ClientState::kConnecting;
    int conn_sk = NonBlockTcpSocket();
    assert(!conn_fdp_);
    conn_fdp_.reset(new PollFd{loop_, conn_sk});
    int err = SocketOp{conn_sk}.Connect(dst_addr_);
    switch (err) {
        case 0:
        case EINTR:  // Does this really happen?
        case EINPROGRESS:
            PrepareConnComp();
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            Retry();
            break;
        case EISCONN:  // This should never be returned.
        default:
            // Clear.
            conn_fdp_.reset();
    }
}

void TcpClientImpl::StopInLoop() {
    loop_.AssertInLoopThread();
    if (state_ == ClientState::kConnecting) {
        LOG_INFO << "TcpClient(" << this << ") stops connecting";
        state_ = ClientState::kDisconnected;
        ClearConnFd(false);
    }
}

void TcpClientImpl::PrepareConnComp() {
    loop_.AssertInLoopThread();
    assert(conn_fdp_);
    assert(state_ == ClientState::kConnecting);
    conn_fdp_->SetWriteCallback([&]() { HandleConnComp(); });
    conn_fdp_->SetErrorCallback([&]() { HandleConnError(); });
    conn_fdp_->EnableWriting();
}

void TcpClientImpl::Retry() {
    loop_.AssertInLoopThread();
    state_ = ClientState::kDisconnected;
    ClearConnFd();
    // It may have been cancelled.
    if (retry_ && connect_) {
        LOG_INFO << "TcpClient(" << this << ") retries connecting";
        // TODO: Implement retrying with timer and it need to be implemented to
        // be cancelable. Note the order of the queued retrying task and clearing
        // fd task.
        // For now, it has to be implemented by shared_from_this().
        loop_.QueueInLoop(
                  std::bind(&TcpClientImpl::ConnectInLoop, shared_from_this()));
    }
}

void TcpClientImpl::HandleConnComp() {
    loop_.AssertInLoopThread();
    // The state may have changed.
    if (state_ != ClientState::kConnecting)
        return;
    if (SocketOp{conn_fdp_->Fd()}.IsSelfConn()) {
        // Self connection.
        LOG_WARN << "TcpClient(" << this << ") has a self connection";
        Retry();
    } else {
        // Disconnect() or StopConnecting() may have been called.
        // This check is not necessary but it can avoid TcpConn object being
        // created.
        if (connect_) {
            int conn_sk = conn_fdp_->DetachFd();
            // This operation must come before ConnEstablished(), or the newly
            // created PollFd object in ConnEstablished() will also be removed.
            ClearConnFd();
            ConnEstablished(conn_sk);
        } else {
            LOG_INFO << "TcpClient(" << this << ") establishes a connection "
                     << "but closes it immediately";
            ClearConnFd();
        }
    }
}

void TcpClientImpl::HandleConnError() {
    loop_.AssertInLoopThread();
    if (state_ != ClientState::kConnecting)
        return;
    assert(conn_fdp_);
    int sk_errno = SocketOp{conn_fdp_->Fd()}.GetError();
    if (sk_errno != 0) {
        LOG_ERROR << "TcpClient(" << this << ") error occurred with errno "
                  << sk_errno << " : " << StrError(sk_errno);
        Retry();
    }
}

void TcpClientImpl::ClearConnFd(bool putoff_reset) {
    loop_.AssertInLoopThread();
    if (conn_fdp_) {
        conn_fdp_->RemoveFromLoop();
        // We may be in the context of conn_fdp_'s HandleEvent().
        // This task should be performed before the retrying task.
        // In some cases, this resetting can not be put off.
        if (putoff_reset) {
            loop_.QueueInLoop(
                [&, this_ptr = shared_from_this()]() { conn_fdp_.reset(); });
        } else {
            conn_fdp_.reset();
        }
    }
}

void TcpClientImpl::ConnEstablished(int conn_sk) {
    loop_.AssertInLoopThread();
    state_ = ClientState::kConnected;
    TcpConnPtr connp = std::make_shared<TcpConn>(loop_, conn_sk);
    LOG_INFO << "TcpClient(" << this << ") establishes the connection and "
             << "creates TcpConn(" << connp.get() << ")";
    // Set callbacks.
    connp->SetConnectedCallback(connnected_cb_);
    connp->SetDisconnectedCallback(disconnected_cb_);
    connp->SetRecvCallback(recv_cb_);
    connp->SetWriteCompCallback(write_comp_cb_);
    // TODO: Using shared_from_this() here will cause a problem, this TcpClient
    // object will not destructs until the connection is closed, which may
    // require calling ForceClose() manually.
    connp->SetCloseCallback(
               std::bind(&TcpClientImpl::HandleConnClose, shared_from_this(), _1));
    // OnConnected().
    connp->OnConnected();
    // This block must be placed last because the closing callback must be
    // guaranteed to be set.
    {
        std::lock_guard<std::mutex> lock{connp_mutex_};
        // Disconnect() or StopConnecting() may have been called.
        if (connect_)
            connp_ = connp;
    }
    // If connp_ is not set to connp, connp will destructs here.
}

void TcpClientImpl::HandleConnClose(TcpConnPtr connp) {
    // connp, i.e., connp_, was bind to loop_.
    loop_.AssertInLoopThread();
    LOG_INFO << "TcpClient(" << this << ") handles TcpConn(" << connp.get()
             << ") closing";
    state_ = ClientState::kDisconnected;
    {
        std::lock_guard<std::mutex> lock{connp_mutex_};
        connp_.reset();
    }
    // Same as what we do in TcpServer, it has to be QueueInLoop().
    loop_.QueueInLoop([=]() { connp->OnDisconnected(); });
    // Retry() checks if it need retry.
    Retry();
}

TcpClient::TcpClient(EventLoop& loop, const InetAddr& dst_addr)
    : pimpl_{std::make_shared<TcpClientImpl>(loop, dst_addr)} {}

void TcpClient::Connect() {
    pimpl_->Connect();
}

void TcpClient::Disconnect() {
    pimpl_->Disconnect();
}

void TcpClient::StopConnecting() {
    pimpl_->StopConnecting();
}

void TcpClient::SetConnectedCallback(ConnectedCallback cb) {
    pimpl_->SetConnectedCallback(std::move(cb));
}

void TcpClient::SetDisconnectedCallback(DisconnectedCallback cb) {
    pimpl_->SetDisconnectedCallback(std::move(cb));
}

void TcpClient::SetRecvCallback(RecvCallback cb) {
    pimpl_->SetRecvCallback(std::move(cb));
}

void TcpClient::SetWriteCompCallback(WriteCompCallback cb) {
    pimpl_->SetWriteCompCallback(std::move(cb));
}

void TcpClient::EnableRetry() {
    pimpl_->EnableRetry();
}

void TcpClient::DisableRetry() {
    pimpl_->DisableRetry();
}

}
