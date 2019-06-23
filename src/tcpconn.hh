#ifndef _AXS_TCPCONN_HH_
#define _AXS_TCPCONN_HH_

#include <memory>
#include <string>
#include <atomic>
#include <boost/core/noncopyable.hpp>

#include "callbacks.hh"
#include "inetaddr.hh"
#include "util/buffer.hh"

namespace axs {

// Forward declaration
class EventLoop;
class PollFd;
class SocketOp;
class TcpConn;
using TcpConnPtr = std::shared_ptr<TcpConn>;

class TcpConn : public std::enable_shared_from_this<TcpConn>,
                private boost::noncopyable {
public:
    using CloseCallback = std::function<void(TcpConnPtr)>;

    TcpConn(EventLoop& loop, int sk, const InetAddr& peer_addr);
    TcpConn(EventLoop& loop, int sk);
    ~TcpConn();

    // Trivial getters.
    EventLoop& OwnerLoop() const { return loop_; }
    int SocketFd() const;
    InetAddr LocalAddr() const { return local_addr_; }
    InetAddr PeerAddr() const { return peer_addr_; }
    bool IsConnected() const { return state_ == ConnState::kConnected; }
    bool IsDisconnected() const { return state_ == ConnState::kDisconnected; }

    // Callback setters.
    void SetConnectedCallback(ConnectedCallback cb) {
        connnected_cb_ = cb; }
    void SetDisconnectedCallback(DisconnectedCallback cb) {
        disconnected_cb_ = cb; }
    void SetRecvCallback(RecvCallback cb) { recv_cb_ = cb; }
    void SetWriteCompCallback(WriteCompCallback cb) {
        write_comp_cb_ = cb; }
    void SetCloseCallback(CloseCallback cb) { close_cb_ = cb; }

    // Thread safe.
    void Send(const std::string& msg);
    // It has the same semantics as the close() system call. Use "Force" to
    // make it clearer.
    void ForceClose();
    void Shutdown();

    // For internal using. Called only once when connection
    // established/destroyed.
    void OnConnected();
    void OnDisconnected();

private:
    enum class ConnState {
        kConnecting, kConnected, kDisconnecting, kDisconnected
    };

    void SendInLoop(const std::string& msg);
    void ForceCloseInLoop();
    void ShutdownInLoop();
    // PollFd event handlers.
    void HandleRecv();
    void HandleSend();
    void HandleClose();
    void HandleError();
    // For logging.
    std::string StateToStr() const;

    EventLoop& loop_;
    std::unique_ptr<PollFd> fdp_;
    std::unique_ptr<SocketOp> sk_opp_;
    std::atomic<ConnState> state_{ConnState::kConnecting};
    // Addresses.
    InetAddr local_addr_;
    InetAddr peer_addr_;
    // Callbacks.
    ConnectedCallback connnected_cb_{};
    DisconnectedCallback disconnected_cb_{};
    RecvCallback recv_cb_{};
    WriteCompCallback write_comp_cb_{};
    CloseCallback close_cb_{};
    // Buffers.
    Buffer recv_buf_{65536};
    Buffer send_buf_{};
};

void DefaultRecvCallback(TcpConnPtr connp, std::string msg);

}
#endif
