#ifndef _AXS_INETADDR_HH_
#define _AXS_INETADDR_HH_

#include <cstdlib>
#include <netinet/in.h>

namespace axs {

class InetAddr {
public:
    InetAddr(const std::string& ip, std::uint16_t port);
    InetAddr(const struct sockaddr_in& addr) : addr_{addr} {}
    const struct sockaddr* SockAddr() const {
        return static_cast<struct sockaddr*>(&addr); }
private:
    struct sockaddr_in addr_{};
};

}
#endif
