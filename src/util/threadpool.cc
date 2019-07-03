#include "threadpool.hh"

namespace axn {

ThreadPool::~ThreadPool() {
    if (running_)
        Stop();
}

void ThreadPool::Start() {
    running_ = true;
    pool_.reserve(thread_num_);
    for (int i = 0; i < thread_num_; ++i) {
        pool_.emplace_back([&]() { ThreadFunc(); });
    }
}

void ThreadPool::Stop() {
    running_ = false;
    not_empty_.notify_all();
    for (auto& t : pool_)
        t.join();
    pool_.clear();
}

void ThreadPool::AddTask(Functor f) {
    std::lock_guard<std::mutex> lock{tasks_mutex_};
    tasks_.push_back(std::move(f));
    not_empty_.notify_one();
}

ThreadPool::Functor ThreadPool::GetTask() {
    std::unique_lock<std::mutex> lock{tasks_mutex_};
    while (tasks_.empty() && running_)
        not_empty_.wait(lock);
    if (!running_)
        return {};
    Functor f = tasks_.front();
    tasks_.pop_front();
    return f;
}

void ThreadPool::ThreadFunc() {
    if (thread_init_cb_)
        thread_init_cb_();
    while (running_) {
        Functor task = GetTask();
        if (task)
            task();
    }
}

}
