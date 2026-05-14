cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Protobuf REQUIRED)

include_directories(
  ${Protobuf_INCLUDE_DIRS}
  /usr/include
  ${CMAKE_CURRENT_BINARY_DIR}
)
link_directories(/usr/local/lib)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/chat.proto)

set(SERVER_SRCS
  src/server.cc
  src/protobuf_codec.cc
  src/chat_service.cc
  ${PROTO_SRCS}
)

add_executable(chat_server ${SERVER_SRCS})

target_link_libraries(chat_server
  muduo_net
  muduo_base
  pthread
  ${Protobuf_LIBRARIES}
)

# 测试客户端
add_executable(test_client
  src/test_client.cc
  src/protobuf_codec.cc
  ${PROTO_SRCS}
)

target_link_libraries(test_client
  muduo_net
  muduo_base
  pthread
  ${Protobuf_LIBRARIES}
)
