#include <cerrno>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "socketop.hh"
#include "inetaddr.hh"
#include "util/log.hh"

namespace axs {

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

void SocketOp::Connect(const InetAddr& addr) {
    if (::connect(sk_, addr.SockAddr(), addr.SockAddrLen()) < 0)
        LOG_ERROR << "Socket " << sk_ << " failed to connect to address "
                  << addr.Ip() << ":" << addr.Port() << " with errno "
                  << errno << " : " << StrError(errno);
}

void SocketOp::Recv(void* buf, std::size_t size) {
    if (::recv(sk_, buf, size, 0) < 0)
        LOG_ERROR << "Recv() on socket " << sk_ << " failed with errno "
                  << errno << " : " << StrError(errno);
}

void SocketOp::Send(void* buf, std::size_t size) {
    if (::send(sk_, buf, size, 0) < 0)
        LOG_ERROR << "Send() on socket " << sk_ << " failed with errno "
                  << errno << " : " << StrError(errno);
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

}
