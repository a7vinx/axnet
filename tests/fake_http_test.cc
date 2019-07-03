#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>

#include "eventloop.hh"
#include "tcpserver.hh"
#include "tcpconn.hh"

using namespace axn;
using std::placeholders::_1;
using std::placeholders::_2;

void FakeHttpRecvCallback(bool keep_alive, TcpConnPtr connp, std::string msg) {
    // Pretend to handle http requests.
    // Use volatile to prevent compiler optimization.
    volatile int counter;
    for (int i = 0; i < 50000; ++i) {
        ++counter;
    }
    std::string fake_http_response{"HTTP/1.1 200 OK\r\n"
                                   "Content-Length: 14\r\n"
                                   "Connection: Keep-Alive\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Server: axnet\r\n\r\n"
                                   "hello, world!\n"};
    connp->Send(fake_http_response);
    if (!keep_alive)
        connp->Shutdown();
}

void fake_http_test(int thread_num, bool keep_alive) {
    EventLoop loop{};
    TcpServer fake_http_server{loop, InetAddr{"127.0.0.1", 9939}};
    fake_http_server.SetThreadNum(thread_num);
    fake_http_server.SetRecvCallback(
                         std::bind(FakeHttpRecvCallback, keep_alive, _1, _2));
    fake_http_server.Start();
    loop.Loop();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: fake_http_test <thread_num> "
                  << "<l/s (long/short connection)>" << std::endl;
        return -1;
    }
    int thread_num = std::atoi(argv[1]);
    bool keep_alive = (argv[2][0] == 'l');
    fake_http_test(thread_num, keep_alive);
    return 0;
}
