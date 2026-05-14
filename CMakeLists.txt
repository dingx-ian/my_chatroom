cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# muduo 头文件路径
include_directories(/usr/include)

# muduo 库文件路径
link_directories(/usr/local/lib)

# 编译可执行文件
add_executable(chat_server src/server.cc)

# 链接 muduo 库和 pthread
target_link_libraries(chat_server muduo_net muduo_base pthread)