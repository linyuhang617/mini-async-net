#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include "../../core/EventLoop.h"
#include "../../net/Connection.h"
#include "../../net/TcpServer.h"

class ChatRoom;

class ChatConnection : public Connection {
public:
    ChatConnection(int fd, std::string name, ChatRoom& room)
        : fd_(fd), name_(std::move(name)), room_(room) {}
    void on_read(int fd) override;
    void on_close() override;
    void send(const std::string& msg) {
        ::write(fd_, msg.c_str(), msg.size());
    }
    const std::string& name() const { return name_; }
private:
    int fd_;
    std::string name_;
    ChatRoom& room_;
};

class ChatRoom {
public:
    void join(std::weak_ptr<ChatConnection> conn) {
        members_.push_back(conn);
    }
    void broadcast(const std::string& msg, ChatConnection* sender) {
        for (auto it = members_.begin(); it != members_.end(); ) {
            auto c = it->lock();
            if (!c) { it = members_.erase(it); continue; }
            if (c.get() != sender) c->send(msg);
            ++it;
        }
    }
    void leave(ChatConnection* conn) {
        broadcast("[" + conn->name() + " left]\n", conn);
    }
private:
    std::vector<std::weak_ptr<ChatConnection>> members_;
};

void ChatConnection::on_read(int fd) {
    char buf[4096];
    while (true) {
        ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n > 0) {
            std::string msg = "[" + name_ + "] " + std::string(buf, n);
            room_.broadcast(msg, this);
        } else if (n == 0 || errno != EAGAIN) {
            disconnected_ = true;
            break;
        } else {
            break;
        }
    }
}

void ChatConnection::on_close() {
    room_.leave(this);
}

int main() {
    EventLoop loop;
    TcpServer server(loop, 9091);
    ChatRoom room;
    int user_id = 0;
    server.start([&room, &user_id](int fd) -> std::shared_ptr<Connection> {
        std::string name = "user" + std::to_string(++user_id);
        auto conn = std::make_shared<ChatConnection>(fd, name, room);
        room.join(conn);
        std::string welcome = "Welcome, " + name + "!\n";
        ::write(fd, welcome.c_str(), welcome.size());
        room.broadcast("[" + name + " joined]\n", conn.get());
        return conn;
    });
    return 0;
}
