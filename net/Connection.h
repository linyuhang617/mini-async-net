#pragma once
#include <vector>
#include <cstddef>

class EventLoop;

class Connection {
public:
    virtual ~Connection() = default;
    virtual void on_read(int fd) = 0;
    virtual void on_close() = 0;
    virtual void on_write(int fd);
    void send(int fd, const char* data, size_t len);
    void set_loop(EventLoop* loop);
    bool is_disconnected() const { return disconnected_; }
protected:
    bool disconnected_ = false;
    std::vector<char> write_buf_;
    EventLoop* loop_ = nullptr;
};
