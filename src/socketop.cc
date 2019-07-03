#include <cerrno>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "socketop.hh"
#include "inetaddr.hh"
#include "util/log.hh"

namespace axn {

void SocketOp::Bind(const InetAddr& addr) {
    if (::bind(sk_, addr.SockAddr(), addr.SockAddrLen()) < 0)
        LOG_FATAL << "Failed to bind socket " << sk_ << " to address "
                  << addr.Ip() << ":" << addr.Port() << " with errno "
                  << errno << " : " << StrError(errno);
}

void SocketOp::Listen() {
    if (::listen(sk_, SOMAXCONN) < 0)
        LOG_FATAL << "Failed to listen on socket " << sk_ << " with errno "
                  << errno << " : " << StrError(errno);
}

std::pair<int, InetAddr> SocketOp::Accept() {
    InetAddr addr{};
    socklen_t addr_len = addr.SockAddrLen();
    int sk = ::accept4(sk_, addr.SockAddr(), &addr_len,
                       SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (sk < 0)
        LOG_ERROR << "Accept() on socket " << sk_ << " failed with errno "
                  << errno << " : " << StrError(errno);
    return {sk, addr};
}

// Errno has to be returned.
int SocketOp::Connect(const InetAddr& addr) {
    int ret = ::connect(sk_, addr.SockAddr(), addr.SockAddrLen());
    if (ret < 0 && errno != EINPROGRESS)
        LOG_ERROR << "Socket " << sk_ << " failed to connect to address "
                  << addr.Ip() << ":" << addr.Port() << " with errno "
                  << errno << " : " << StrError(errno);
    return ret < 0 ? errno : 0;
}

ssize_t SocketOp::Recv(void* buf, std::size_t size) {
    ssize_t n = ::recv(sk_, buf, size, 0);
    if (n < 0)
        LOG_ERROR << "Recv() on socket " << sk_ << " failed with errno "
                  << errno << " : " << StrError(errno);
    return n;
}

ssize_t SocketOp::Send(const void* buf, std::size_t size) {
    ssize_t n = ::send(sk_, buf, size, 0);
    if (n < 0)
        LOG_ERROR << "Send() on socket " << sk_ << " failed with errno "
                  << errno << " : " << StrError(errno);
    return n;
}

void SocketOp::ShutdownWrite() {
    if (::shutdown(sk_, SHUT_WR) < 0)
        LOG_ERROR << "Failed to shut down writing on socket " << sk_
                  << " with errno " << errno << " : " << StrError(errno);
}

void SocketOp::SetTcpNoDelay(bool val) {
    int val_int = val;
    int ret = ::setsockopt(sk_, IPPROTO_TCP, TCP_NODELAY, &val_int,
                           static_cast<socklen_t>(sizeof(val_int)));
    if (ret < 0)
        LOG_ERROR << "SetTcpNoDelay() on socket " << sk_
                  << " failed with errno " << errno << " : " << StrError(errno);
}

void SocketOp::SetReuseAddr(bool val) {
    int val_int = val;
    int ret = ::setsockopt(sk_, SOL_SOCKET, SO_REUSEADDR, &val_int,
                           static_cast<socklen_t>(sizeof(val_int)));
    if (ret < 0)
        LOG_ERROR << "SetReuseAddr() on socket " << sk_
                  << " failed with errno " << errno << " : " << StrError(errno);
}

void SocketOp::SetReusePort(bool val) {
    int val_int = val;
    int ret = ::setsockopt(sk_, SOL_SOCKET, SO_REUSEPORT, &val_int,
                           static_cast<socklen_t>(sizeof(val_int)));
    if (ret < 0)
        LOG_ERROR << "SetReusePort() on socket " << sk_
                  << " failed with errno " << errno << " : " << StrError(errno);
}

void SocketOp::SetKeepAlive(bool val) {
    int val_int = val;
    int ret = ::setsockopt(sk_, SOL_SOCKET, SO_KEEPALIVE, &val_int,
                           static_cast<socklen_t>(sizeof(val_int)));
    if (ret < 0)
        LOG_ERROR << "SetKeepAlive() on socket " << sk_
                  << " failed with errno " << errno << " : " << StrError(errno);
}

InetAddr SocketOp::GetLocalAddr() const {
    InetAddr local_addr{};
    socklen_t addr_len = local_addr.SockAddrLen();
    if (::getsockname(sk_, local_addr.SockAddr(), &addr_len) < 0)
        LOG_ERROR << "Failed to get local address on socket " << sk_
                  << " with errno " << errno << " : " << StrError(errno);
    return local_addr;
}

InetAddr SocketOp::GetPeerAddr() const {
    InetAddr peer_addr{};
    socklen_t addr_len = peer_addr.SockAddrLen();
    if (::getpeername(sk_, peer_addr.SockAddr(), &addr_len) < 0)
        LOG_ERROR << "Failed to get peer address on socket " << sk_
                  << " with errno " << errno << " : " << StrError(errno);
    return peer_addr;
}

int SocketOp::GetError() const {
    int val_int;
    socklen_t val_len = static_cast<socklen_t>(sizeof(val_int));
    if (::getsockopt(sk_, SOL_SOCKET, SO_ERROR, &val_int, &val_len) < 0) {
        LOG_ERROR << "Failed to get socket error on socket " << sk_
                  << " with errno " << errno << " : " << StrError(errno);
        return 0;
    } else {
        return val_int;
    }
}

bool SocketOp::IsSelfConn() const {
    InetAddr local_addr = GetLocalAddr();
    InetAddr peer_addr = GetPeerAddr();
    return local_addr.Port() == peer_addr.Port() &&
           local_addr.Ip() == peer_addr.Ip();
}

int NonBlockTcpSocket() {
    int sk = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                      IPPROTO_TCP);
    if (sk < 0)
        LOG_FATAL << "Failed to create socket with errno " << errno << " : "
                  << StrError(errno);
    return sk;
}

}
