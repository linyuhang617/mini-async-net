#include "EventLoop.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

EventLoop::EventLoop() {
    epoll_fd_ = ::epoll_create1(0);
    if (epoll_fd_ < 0) throw std::runtime_error("epoll_create1 failed");
}

EventLoop::~EventLoop() {
    ::close(epoll_fd_);
}

void EventLoop::add_handler(int fd, uint32_t events, std::function<void()> cb) {
    handlers_[fd] = std::move(cb);
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

void EventLoop::remove_handler(int fd) {
    handlers_.erase(fd);
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

void EventLoop::run() {
    epoll_event events[MAX_EVENTS];
    while (true) {
        int n = ::epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
        if (n < 0) break;
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            auto it = handlers_.find(fd);
            if (it != handlers_.end()) {
                auto cb = it->second; // copy before calling
                cb();
            }
        }
    }
}
