#pragma once
#include <functional>
#include <unordered_map>
#include <sys/epoll.h>
#include <stdexcept>

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void add_handler(int fd, uint32_t events, std::function<void()> cb);
    void remove_handler(int fd);
    void run();

private:
    int epoll_fd_;
    std::unordered_map<int, std::function<void()>> handlers_;
    static const int MAX_EVENTS = 64;
};
