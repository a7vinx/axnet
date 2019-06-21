#ifndef _AXS_SOCKETOP_HH_
#define _AXS_SOCKETOP_HH_

#include <utility>
#include <cstdlib>
#include <sys/types.h>

namespace axs {

// Forward declaration.
class InetAddr;

// Socket operator. It is not responsible for the life cycle of the socket.
class SocketOp {
public:
    explicit SocketOp(int sk) : sk_{sk} {}

    // Normal operations.
    // Abort if failed.
    void Bind(const InetAddr& addr);
    // Abort if failed.
    void Listen();
    // Return the file descriptor referring to the newly created socket and
    // an InetAddr object representing the peer address. On error, the file
    // descriptor will be set to -1 and the InetAddr object will be invalid.
    std::pair<int, InetAddr> Accept();
    int Connect(const InetAddr& addr);
    ssize_t Recv(void* buf, std::size_t size);
    ssize_t Send(const void* buf, std::size_t size);
    void ShutdownWrite();

    // Socket options wrappers.
    void SetTcpNoDelay(bool val);
    void SetReuseAddr(bool val);
    void SetReusePort(bool val);
    void SetKeepAlive(bool val);

    // Get information.
    InetAddr GetLocalAddr() const;
    InetAddr GetPeerAddr() const;
    int GetError() const;
    bool IsSelfConn() const;

private:
    int sk_;
};

// Abort if failed.
int NonBlockTcpSocket();

}
#endif
