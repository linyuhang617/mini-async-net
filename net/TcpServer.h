#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include "../core/EventLoop.h"
#include "Connection.h"

class TcpServer {
public:
    using Factory = std::function<std::shared_ptr<Connection>(int fd)>;

    TcpServer(EventLoop& loop, int port);
    ~TcpServer();

    void start(Factory factory);

private:
    void set_nonblocking(int fd);

    EventLoop& loop_;
    int listen_fd_;
    std::unordered_map<int, std::shared_ptr<Connection>> conn_map_;
};
