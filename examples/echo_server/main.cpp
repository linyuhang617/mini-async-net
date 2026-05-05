#include <iostream>
#include <memory>
#include <unistd.h>
#include "../../core/EventLoop.h"
#include "../../net/Connection.h"
#include "../../net/TcpServer.h"

class EchoConnection : public Connection {
public:
    void on_read(int fd) override {
        char buf[4096];
        while (true) {
            ssize_t n = ::read(fd, buf, sizeof(buf));
            if (n > 0) {
                ::write(fd, buf, n);
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
    EventLoop loop;
    TcpServer server(loop, 9090);
    server.start([](int fd) {
        return std::make_shared<EchoConnection>();
    });
    return 0;
}
