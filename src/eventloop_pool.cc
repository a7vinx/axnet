#include <cassert>

#include "eventloop_pool.hh"
#include "eventloop.hh"

namespace axn {

EventLoopPool::~EventLoopPool() {
    // std::thread can only be destructed after being joined or detached.
    if (running_)
        Stop();
}

void EventLoopPool::SetThreadNum(int n) {
    assert(n >= 0);
    thread_num_ = n;
}

void EventLoopPool::Start() {
    running_ = true;
    for (int i = 0; i < thread_num_; ++i) {
        thread_pool_.emplace_back([&]() { LoopThreadFunc(); });
    }
    std::unique_lock<std::mutex> lock{loop_pool_mutex_};
    while (loop_pool_.size() != thread_num_) {
        loop_pool_ready_.wait(lock);
    }
}

void EventLoopPool::Stop() {
    running_ = false;
    for (auto loopp : loop_pool_) {
        loopp->Quit();
    }
    for (auto& t : thread_pool_) {
        t.join();
    }
    thread_pool_.clear();
    loop_pool_.clear();
}

EventLoop& EventLoopPool::GetNextLoop() {
    if (thread_num_ == 0) {
        return loop_;
    } else {
        next_id_ = (next_id_ + 1) % thread_num_;
        return *loop_pool_[next_id_];
    }
}

void EventLoopPool::LoopThreadFunc() {
    EventLoop loop;
    {
        std::lock_guard<std::mutex> lock{loop_pool_mutex_};
        loop_pool_.push_back(&loop);
    }
    loop_pool_ready_.notify_one();
    loop.Loop();
}

}
