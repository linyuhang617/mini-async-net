#pragma once
#include <functional>
#include <unordered_map>
#include <sys/epoll.h>
#include <stdexcept>
#include <sys/eventfd.h>
#include <queue>
#include <mutex>

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void add_handler(int fd, uint32_t events, std::function<void(uint32_t)> cb);
    void remove_handler(int fd);
    void run();
    void stop();
    void modify_handler(int fd, uint32_t new_events);
    void post(std::function<void()> fn);

private:
    int epoll_fd_;
    int stop_fd_;
    bool stop_ = false;
    struct Handler {
        uint32_t events;
        std::function<void(uint32_t)> cb;
    };
    std::unordered_map<int, Handler> handlers_;
    std::queue<std::function<void()>> pending_;
    std::mutex pending_mutex_;
    static const int MAX_EVENTS = 64;
};
