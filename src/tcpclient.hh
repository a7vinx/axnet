#ifndef _AXS_TCPCLIENT_HH_
#define _AXS_TCPCLIENT_HH_

#include <memory>
#include <boost/core/noncopyable.hpp>

#include "callbacks.hh"

namespace axs {

// Forward declaration.
class EventLoop;
class InetAddr;
class TcpClientImpl;

class TcpClient : private boost::noncopyable {
public:
    TcpClient(EventLoop& loop, const InetAddr& dst_addr);

    // Thread safe.
    void Connect();
    // Shut down an established connection. If it is connecting, the
    // connection will be closed as soon as it is established.
    // Thread safe.
    void Disconnect();
    // Stop connecting if the connection has not been established yet.
    // Threas safe.
    void StopConnecting();

    // Callback setters.
    void SetConnectedCallback(ConnectedCallback cb);
    void SetDisconnectedCallback(DisconnectedCallback cb);
    void SetRecvCallback(RecvCallback cb);
    void SetWriteCompCallback(WriteCompCallback cb);

    // Others.
    void EnableRetry();
    void DisableRetry();

private:
    std::shared_ptr<TcpClientImpl> pimpl_;
};

}
#endif
