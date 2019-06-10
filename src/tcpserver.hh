#ifndef _AXS_TCPSERVER_HH_
#define _AXS_TCPSERVER_HH_

#include <map>
#include <memory>
#include <boost/core/noncopyable.hpp>

#include "callbacks.hh"
#include "tcpconn.hh"

namespace axs {

// Forward declaration.
class EventLoop;
class EventLoopPool;
class Acceptor;
class InetAddr;

class TcpServer : private boost::noncopyable {
public:
    TcpServer(EventLoop& loop, const InetAddr& addr);
    ~TcpServer();

    // 0 means that all I/O will be done in the provided loop.
    // n means that all new connections will be assigned to the loop pool of
    // size n in a round-robin way.
    void SetThreadNum(int n);
    void Start();

    // Callback setters.
    void SetConnectedCallback(ConnectedCallback cb) {
        connnected_cb_ = cb; }
    void SetDisconnectedCallback(DisconnectedCallback cb) {
        disconnected_cb_ = cb; }
    void SetRecvCallback(RecvCallback cb) { recv_cb_ = cb; }
    void SetWriteCompCallback(WriteCompCallback cb) {
        write_comp_cb_ = cb; }

private:
    void HandleNewConn(int sk, const InetAddr& peer_addr);
    void HandleConnClose(TcpConnPtr connp);
    void HandleConnCloseInLoop(TcpConnPtr connp);

    EventLoop& loop_;
    std::unique_ptr<EventLoopPool> loop_poolp_;
    std::unique_ptr<Acceptor> acceptorp_;
    // Socket - TcpConnPtr map.
    std::map<int, TcpConnPtr> conns_;
    // Callbacks.
    ConnectedCallback connnected_cb_{};
    DisconnectedCallback disconnected_cb_{};
    RecvCallback recv_cb_{DefaultRecvCallback};
    WriteCompCallback write_comp_cb_{};
};

}
#endif
