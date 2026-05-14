#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include "chat.pb.h"

namespace chat
{

    using muduo::Timestamp;
    using muduo::net::TcpConnectionPtr;

    class ChatService
    {
    public:
        ChatService();

        void onConnection(const TcpConnectionPtr &conn);
        void onMessage(const TcpConnectionPtr &conn, const chatroom::ChatMessage &message, Timestamp receiveTime);

    private:
        void addUser(const std::string &user, const TcpConnectionPtr &conn);
        void removeUser(const TcpConnectionPtr &conn);
        void broadcastMessage(const chatroom::ChatMessage &message);
        void sendPrivateMessage(const std::string &to, const chatroom::ChatMessage &message);

        std::unordered_map<std::string, TcpConnectionPtr> userConnections_;
        std::mutex mutex_;
    };

} // namespace chat
