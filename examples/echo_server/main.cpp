#include <iostream>
#include <memory>
#include <string>
#include <csignal>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "../../core/EventLoop.h"
#include "../../core/ThreadPool.h"
#include "../../net/Connection.h"
#include "../../net/TcpServer.h"

static ThreadPool pool(4);

class EchoConnection : public Connection,
                       public std::enable_shared_from_this<EchoConnection> {
public:
    void on_read(int fd) override {
        char buf[4096];
        while (true) {
            ssize_t n = ::read(fd, buf, sizeof(buf));
            if (n > 0) {
                std::string data(buf, n);
                std::weak_ptr<EchoConnection> weak = weak_from_this();
                EventLoop* lp = loop_;
                pool.submit([weak, lp, fd, data]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    lp->post([weak, fd, data]() {
                        auto self = weak.lock();
                        if (!self || self->is_disconnected()) return;
                        self->send(fd, data.c_str(), data.size());
                    });
                });
            } else if (n == 0 || errno != EAGAIN) {
                disconnected_ = true;
                break;
            } else {
                break;
            }
        }
    }
    void on_close() override {}
};

int main() {
    signal(SIGPIPE, SIG_IGN);
    EventLoop loop;
    TcpServer server(loop, 9090);
    server.start([](int fd) {
        return std::make_shared<EchoConnection>();
    });
    return 0;
}
