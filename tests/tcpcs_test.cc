#include <thread>
#include <chrono>
#include <iostream>

#include "eventloop.hh"
#include "tcpclient.hh"
#include "tcpserver.hh"
#include "tcpconn.hh"

using namespace axs;

void ClientConnected(TcpConnPtr connp) {
    connp->Send("hello");
    std::cout << "Client: Connected" << std::endl;
}

void EchoRecvCallback(TcpConnPtr connp, std::string msg) {
    connp->Send(msg);
    std::cout << "Server: Receive message: "  << msg << std::endl;
}

void ClientCtl(EventLoop* lp) {
    TcpClient client{*lp, InetAddr{"127.0.0.1", 9939}};
    client.SetConnectedCallback(ClientConnected);
    // Use default message callback.
    client.Connect();
    client.StopConnecting();
    // If StopConnecting() works, this will try connect again, or it will do
    // nothing.
    client.Connect();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    client.Disconnect();
    std::this_thread::sleep_for(1s);
    // Connect again.
    client.Connect();
    std::this_thread::sleep_for(1s);
    client.Disconnect();
    std::this_thread::sleep_for(1s);
    // Quit.
    lp->Quit();
}

int main() {
    EventLoop loop;
    InetAddr server_addr{"127.0.0.1", 9939};
    TcpServer server{loop, server_addr};
    server.SetRecvCallback(EchoRecvCallback);
    server.Start();
    std::thread client_ctl_thread{ClientCtl, &loop};
    loop.Loop();
    client_ctl_thread.join();
}
