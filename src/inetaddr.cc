#include <endian.h>
#include <arpa/inet.h>

#include "inetaddr.hh"
#include "util/log.hh"

namespace axs {

static_assert(sizeof(InetAddr) == sizeof(struct sockaddr_in),
              "InetAddr and struct sockaddr_in are different sizes");

InetAddr::InetAddr(const std::string& ip, std::uint16_t port) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htobe16(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) < 0)
        LOG_ERROR << "Illegal ip address: " << ip;
}

std::uint16_t InetAddr::Port() const {
    return be16toh(addr_.sin_port);
}

std::string InetAddr::Ip() const {
    char ip[64];
    ::inet_ntop(AF_INET, &addr_.sin_addr, ip, sizeof(ip));
    return ip;
}

}
