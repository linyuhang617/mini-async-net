#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <csignal>
#include <algorithm>
#include <unistd.h>
#include "../../core/EventLoop.h"
#include "../../net/Connection.h"
#include "../../net/TcpServer.h"

class ChatRoom;

class ChatConnection : public Connection {
public:
    ChatConnection(std::string name, ChatRoom& room)
        : name_(std::move(name)), room_(room) {}
    void on_read(int fd) override;
    void on_close() override;
    void send(int fd, const std::string& msg) {
        Connection::send(fd, msg.c_str(), msg.size());
    }
    const std::string& name() const { return name_; }
private:
    std::string name_;
    ChatRoom& room_;
};

class ChatRoom {
public:
    void join(int fd, std::weak_ptr<ChatConnection> conn) {
        members_.push_back({fd, conn});
    }
    void broadcast(const std::string& msg, ChatConnection* sender) {
        for (auto it = members_.begin(); it != members_.end(); ) {
            auto c = it->second.lock();
            if (!c) { it = members_.erase(it); continue; }
            if (c.get() != sender) c->send(it->first, msg);
            ++it;
        }
    }
    void leave(ChatConnection* conn) {
        broadcast("[" + conn->name() + " left]\n", conn);
        members_.erase(
            std::remove_if(members_.begin(), members_.end(),
                [conn](const std::pair<int, std::weak_ptr<ChatConnection>>& p) {
                    auto c = p.second.lock();
                    return !c || c.get() == conn;
                }),
            members_.end()
        );
    }
private:
    std::vector<std::pair<int, std::weak_ptr<ChatConnection>>> members_;
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
    signal(SIGPIPE, SIG_IGN);
    EventLoop loop;
    TcpServer server(loop, 9091);
    ChatRoom room;
    int user_id = 0;
    server.start([&room, &user_id](int fd) -> std::shared_ptr<Connection> {
        std::string name = "user" + std::to_string(++user_id);
        auto conn = std::make_shared<ChatConnection>(name, room);
        room.join(fd, conn);
        std::string welcome = "Welcome, " + name + "!\n";
        { [[maybe_unused]] auto r = ::write(fd, welcome.c_str(), welcome.size()); }
        room.broadcast("[" + name + " joined]\n", conn.get());
        return conn;
    });
    return 0;
}
