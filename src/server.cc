#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <iostream>
#include <functional>
#include "protobuf_codec.h"
#include "chat_service.h"

int main(int argc, char *argv[])
{
    muduo::Logger::setLogLevel(muduo::Logger::INFO);
    LOG_INFO << "Chat server starting...";

    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr(8888);

    chat::ChatService chatService;
    chat::ProtobufCodec codec(
        std::bind(&chat::ChatService::onMessage,
                  &chatService,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));

    muduo::net::TcpServer server(&loop, listenAddr, "ChatServer");

    server.setConnectionCallback(
        std::bind(&chat::ChatService::onConnection,
                  &chatService,
                  std::placeholders::_1));

    server.setMessageCallback(
        std::bind(&chat::ProtobufCodec::onMessage,
                  &codec,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));

    server.start();
    std::cout << "[Server] Listening on port 8888..." << std::endl;

    loop.loop();
    return 0;
}
