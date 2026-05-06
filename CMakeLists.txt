cmake_minimum_required(VERSION 3.14)
project(mini-async-net CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)

find_package(Threads REQUIRED)

add_library(net_core STATIC
    core/EventLoop.cpp
    core/ThreadPool.cpp
    net/Connection.cpp
    net/TcpServer.cpp
)
target_link_libraries(net_core Threads::Threads)

add_executable(echo_server examples/echo_server/main.cpp)
target_link_libraries(echo_server net_core)

add_executable(echo_server_plain examples/echo_server/main_plain.cpp)
target_link_libraries(echo_server_plain net_core)

add_executable(chat_server examples/chat_server/main.cpp)
target_link_libraries(chat_server net_core)

add_executable(bench benchmark/bench.cpp)
target_link_libraries(bench Threads::Threads)
