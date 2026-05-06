#include "Connection.h"
#include "../core/EventLoop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>

void Connection::set_loop(EventLoop* loop) {
    loop_ = loop;
}

void Connection::send(int fd, const char* data, size_t len) {
    if (write_buf_.empty()) {
        ssize_t n = ::write(fd, data, len);
        if (n < 0) n = 0;
        if ((size_t)n < len) {
            write_buf_.insert(write_buf_.end(), data + n, data + len);
            if (loop_) loop_->modify_handler(fd, EPOLLIN | EPOLLOUT | EPOLLET);
        }
    } else {
        write_buf_.insert(write_buf_.end(), data, data + len);
    }
}

void Connection::on_write(int fd) {
    while (!write_buf_.empty()) {
        ssize_t n = ::write(fd, write_buf_.data(), write_buf_.size());
        if (n > 0) {
            write_buf_.erase(write_buf_.begin(), write_buf_.begin() + n);
        } else {
            break;
        }
    }
    if (write_buf_.empty() && loop_) {
        loop_->modify_handler(fd, EPOLLIN | EPOLLET);
    }
}
