#ifndef _AXS_SOCKETOP_HH_
#define _AXS_SOCKETOP_HH_

#include <utility>
#include <cstdlib>

namespace axs {

// Forward declaration.
class InetAddr;

// Socket operator. It is not responsible for the life cycle of the socket.
class SocketOp {
public:
    explicit SocketOp(int sk) : sk_{sk} {}
    void Bind(const InetAddr& addr);
    void Listen();
    // Return the file descriptor referring to the newly created socket and
    // an InetAddr object representing the peer address. On error, the file
    // descriptor will be set to -1 and the InetAddr object will be invalid.
    std::pair<int, InetAddr> Accept();
    void Connect(const InetAddr& addr);
    void Recv(void* buf, std::size_t size);
    void Send(void* buf, std::size_t size);

    // Socket options wrappers.
    void SetTcpNoDelay(bool val);
    void SetReuseAddr(bool val);
    void SetReusePort(bool val);
    void SetKeepAlive(bool val);

private:
    int sk_;
};

}
#endif
