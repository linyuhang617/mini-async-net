#pragma once

class Connection {
public:
    virtual ~Connection() = default;
    virtual void on_read(int fd) = 0;
    virtual void on_close() = 0;
    bool is_disconnected() const { return disconnected_; }
protected:
    bool disconnected_ = false;
};
