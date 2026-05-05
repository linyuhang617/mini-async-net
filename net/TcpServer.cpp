#include "TcpServer.h"
#include <stdexcept>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

TcpServer::TcpServer(EventLoop& loop, int port) : loop_(loop) {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) throw std::runtime_error("socket failed");

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");
    if (::listen(listen_fd_, 128) < 0)
        throw std::runtime_error("listen failed");

    set_nonblocking(listen_fd_);
}

TcpServer::~TcpServer() {
    ::close(listen_fd_);
}

void TcpServer::set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void TcpServer::start(Factory factory) {
    loop_.add_handler(listen_fd_, EPOLLIN, [this, factory]() {
        while (true) {
            int conn_fd = ::accept(listen_fd_, nullptr, nullptr);
            if (conn_fd < 0) break;
            set_nonblocking(conn_fd);
            std::cout << "client connected fd=" << conn_fd << "\n";

            auto conn = factory(conn_fd);
            conn_map_[conn_fd] = conn;

            loop_.add_handler(conn_fd, EPOLLIN | EPOLLET, [this, conn_fd]() {
                auto it = conn_map_.find(conn_fd);
                if (it == conn_map_.end()) return;
                auto& conn = it->second;
                conn->on_read(conn_fd);
                if (conn->is_disconnected()) {
                    std::cout << "client disconnected fd=" << conn_fd << "\n";
                    conn->on_close();
                    loop_.remove_handler(conn_fd);
                    conn_map_.erase(conn_fd);
                    ::close(conn_fd);
                }
            });
        }
    });

    std::cout << "server listening (epoll + TcpServer)\n";
    loop_.run();
}
