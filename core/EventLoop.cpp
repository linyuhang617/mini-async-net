#include "EventLoop.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

EventLoop::EventLoop() {
    epoll_fd_ = ::epoll_create1(0);
    if (epoll_fd_ < 0) throw std::runtime_error("epoll_create1 failed");
    stop_fd_ = ::eventfd(0, EFD_NONBLOCK);
    if (stop_fd_ < 0) throw std::runtime_error("eventfd failed");
    epoll_event sev{};
    sev.events = EPOLLIN;
    sev.data.fd = stop_fd_;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, stop_fd_, &sev);
}

EventLoop::~EventLoop() {
    ::close(stop_fd_);
    ::close(epoll_fd_);
}

void EventLoop::add_handler(int fd, uint32_t events, std::function<void(uint32_t)> cb) {
    handlers_[fd] = {events, std::move(cb)};
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
        if (n < 0 && errno != EINTR) break;
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == stop_fd_) {
                uint64_t val;
                [[maybe_unused]] auto rr = ::read(stop_fd_, &val, sizeof(val));
                std::queue<std::function<void()>> local;
                {
                    std::lock_guard<std::mutex> lk(pending_mutex_);
                    std::swap(local, pending_);
                }
                while (!local.empty()) { local.front()(); local.pop(); }
                if (stop_) break;
                continue;
            }
            auto it = handlers_.find(fd);
            if (it != handlers_.end()) {
                auto cb = it->second.cb; // copy before calling
                cb(events[i].events);
            }
        }
    }
}

void EventLoop::stop() {
    stop_ = true;
    uint64_t val = 1;
    [[maybe_unused]] auto r = ::write(stop_fd_, &val, sizeof(val));
}

void EventLoop::post(std::function<void()> fn) {
    {
        std::lock_guard<std::mutex> lk(pending_mutex_);
        pending_.push(std::move(fn));
    }
    uint64_t val = 1;
    [[maybe_unused]] auto r = ::write(stop_fd_, &val, sizeof(val));
}

void EventLoop::modify_handler(int fd, uint32_t new_events) {
    auto it = handlers_.find(fd);
    if (it == handlers_.end()) throw std::runtime_error("modify_handler: fd not found");
    it->second.events = new_events;
    epoll_event ev{};
    ev.events = new_events;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}
