#ifndef _AXS_CALLBACKS_HH_
#define _AXS_CALLBACKS_HH_

#include <functional>
#include <memory>
#include <string>

namespace axs {

class TcpConn;
using TcpConnPtr = std::shared_ptr<TcpConn>;
using ConnectedCallback = std::function<void(TcpConnPtr)>;
using DisconnectedCallback = std::function<void(TcpConnPtr)>;
using RecvCallback = std::function<void(TcpConnPtr, std::string)>;
using WriteCompCallback = std::function<void(TcpConnPtr)>;

}
#endif
