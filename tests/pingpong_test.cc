#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <fstream>
#include <boost/ptr_container/ptr_vector.hpp>

#include "eventloop.hh"
#include "eventloop_pool.hh"
#include "tcpserver.hh"
#include "tcpclient.hh"
#include "tcpconn.hh"

using namespace axn;
using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

InetAddr server_addr{"127.0.0.1", 9939};
std::mutex total_sent_size_mutex{};
std::size_t total_sent_size = 0;
int completed_clients = 0;


// Client Callbacks.
void ClientOnConnected(const std::string& init_msg, TcpConnPtr connp) {
    connp->Send(init_msg);
}

void ClientEcho(std::size_t* sent_sizep, TcpConnPtr connp, std::string msg) {
    connp->Send(msg);
    *sent_sizep += msg.size();
}

void ClientOnDisconnected(int conn_num, std::size_t* sent_sizep,
                          TcpConnPtr connp) {
    std::lock_guard<std::mutex> lock{total_sent_size_mutex};
    ++completed_clients;
    total_sent_size += *sent_sizep;
    if (completed_clients == conn_num) {
        std::cout << "Total sent: " << total_sent_size << " bytes" << std::endl;
        std::cout << "Throughput: "
                  << total_sent_size / (1024.0f * 1024 * 60) << " MiB/s"
                  << std::endl;
    }
}

// Server Callbacks.
void ServerEcho(TcpConnPtr connp, std::string msg) {
    connp->Send(msg);
}

void StartServer(int thread_num, EventLoop** loop_addrp) {
    EventLoop server_main_loop{};
    *loop_addrp = &server_main_loop;
    TcpServer server{server_main_loop, server_addr};
    server.SetThreadNum(thread_num);
    server.SetRecvCallback(ServerEcho);
    server.Start();
    server_main_loop.Loop();
}

void ServerCtl(int thread_num) {
    EventLoop* loopp = nullptr;
    std::thread server_thread{StartServer, thread_num, &loopp};
    // Little longer than clients. Let clients close the connections.
    std::this_thread::sleep_for(63s);
    loopp->Quit();
    server_thread.join();
}

std::string GetInitMsg(std::size_t block_size) {
    std::ifstream random_ifs("/dev/urandom");
    if (!random_ifs.is_open()) {
        std::cout << "Failed to open /dev/urandom" << std::endl;
        return {};
    }
    std::string random_str(block_size, ' ');
    random_ifs.read(&random_str[0], block_size);
    return random_str;
}

void ClientCtl(int thread_num, int conn_num, std::size_t block_size) {
    // loop is only used to create EventLoopPool and it is not an assignable
    // loop.
    EventLoop loop{};
    // Use EventLoopPool so we do not need another single thread like
    // ServerCtl().
    // EventLoopPool is not meant for this but we want to take the easy way out.
    EventLoopPool loop_pool{loop};
    // Not like StartServer, because loop itself is not assignable, use
    // thread_num here.
    loop_pool.SetThreadNum(thread_num);
    // Statistics.
    std::vector<std::size_t> clients_statistics(conn_num, block_size);
    std::string init_msg = GetInitMsg(block_size);
    loop_pool.Start();
    boost::ptr_vector<TcpClient> clients{};
    for (int i = 0; i < conn_num; ++i) {
        clients.push_back(new TcpClient{loop_pool.GetNextLoop(), server_addr});
        clients[i].SetConnectedCallback(
                   std::bind(ClientOnConnected, init_msg, _1));
        clients[i].SetDisconnectedCallback(
                   std::bind(ClientOnDisconnected, conn_num,
                             &clients_statistics[i], _1));
        clients[i].SetRecvCallback(
                   std::bind(ClientEcho, &clients_statistics[i], _1, _2));
        clients[i].Connect();
    }
    std::this_thread::sleep_for(60s);
    for (auto& client : clients) {
        client.Disconnect();
    }
    // Wait for clients to disconnect.
    std::this_thread::sleep_for(1s);
    // Can't call Stop() manually here because we need clients' loops destructs
    // after clients.
    // loop_pool.Stop();
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cout << "Usage: pingpong_test <server_thread_num> "
                  << "<client_thread_num> <connection_num> "
                  << "<block_size>" << std::endl;
        return 1;
    }
    int server_thread_num = std::atoi(argv[1]);
    int client_thread_num = std::atoi(argv[2]);
    int conn_num = std::atoi(argv[3]);
    std::size_t block_size = std::atoll(argv[4]);
    std::thread server_ctl_thread{ServerCtl, server_thread_num};
    // Leave 1s for server's starting.
    std::this_thread::sleep_for(1s);
    std::thread client_ctl_thread{ClientCtl, client_thread_num, conn_num, block_size};
    server_ctl_thread.join();
    client_ctl_thread.join();
    return 0;
}
