cmake_minimum_required(VERSION 3.14)
project(mini-async-net CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

add_library(net_core STATIC
    core/EventLoop.cpp
    net/Connection.cpp
    net/TcpServer.cpp
)

add_executable(echo_server examples/echo_server/main.cpp)
target_link_libraries(echo_server net_core)

add_executable(chat_server examples/chat_server/main.cpp)
target_link_libraries(chat_server net_core)
