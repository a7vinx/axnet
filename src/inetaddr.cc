#include <endian.h>
#include <arpa/inet.h>

#include "inetaddr.hh"
#include "util/log.hh"

namespace axs {

InetAddr::InetAddr(const std::string& ip, std::uint16_t port) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htobe16(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) < 0)
        LOG_ERROR << "Illegal ip address: " << ip;
}

}
