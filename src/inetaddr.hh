#ifndef _AXS_INETADDR_HH_
#define _AXS_INETADDR_HH_

#include <string>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>

namespace axs {

class InetAddr {
public:
    // Construct an empty struct sockaddr_in used to be filled by accept() etc.
    InetAddr() {}
    InetAddr(const std::string& ip, std::uint16_t port);
    InetAddr(const struct sockaddr_in& addr) : addr_{addr} {}
    const struct sockaddr* SockAddr() const {
        return reinterpret_cast<const struct sockaddr*>(&addr_); }
    struct sockaddr* SockAddr() {
        return reinterpret_cast<struct sockaddr*>(&addr_); }
    socklen_t SockAddrLen() const {
        return static_cast<socklen_t>(sizeof(addr_)); }
    // Get printable address.
    std::uint16_t Port() const;
    std::string Ip() const;
private:
    struct sockaddr_in addr_{};
};

}
#endif
