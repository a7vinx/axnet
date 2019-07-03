#include <iostream>
#include <cstdlib>
#include <sys/eventfd.h>

#include "eventloop.hh"
#include "pollfd.hh"

using namespace axn;

void AssertInOtherThread(EventLoop* lp) {
    lp->AssertInLoopThread();
}

void AssertionTest() {
    EventLoop l1;
    // EventLoop l2;  // Abort
    l1.AssertInLoopThread();
    std::thread t{AssertInOtherThread, &l1};
    t.join();
}

void Output(const std::string& str) {
    std::cout << str << std::endl;
}

void HandleCtlFdReading(int ctl_fd) {
    std::uint64_t one = 0;
    Output("reading from ctl fd");
    ::read(ctl_fd, &one, sizeof(one));
}

void LoopOnce(int ctl_fd) {
    std::uint64_t one;
    ::write(ctl_fd, &one, sizeof(one));
}

void CtlLoopThread(EventLoop* lp, PollFd* fdp) {
    lp->QueueInLoop([]() { Output("QueueInLoop1"); });
    lp->QueueInLoop([]() { Output("QueueInLoop2"); });
    LoopOnce(fdp->Fd());
    LoopOnce(fdp->Fd());
    lp->QueueInLoop([]() { Output("QueueInLoop3"); });
    lp->Quit();
    LoopOnce(fdp->Fd());
}

void LoopTest() {
    EventLoop l;
    PollFd ctl_fd{l, ::eventfd(0, EFD_NONBLOCK)};
    ctl_fd.SetReadCallback([&](){ HandleCtlFdReading(ctl_fd.Fd()); });
    ctl_fd.EnableReading();
    std::thread t{CtlLoopThread, &l, &ctl_fd};
    l.Loop();
    t.join();
    ctl_fd.RemoveFromLoop();
}

int main() {
    // AssertionTest();
    LoopTest();
}
