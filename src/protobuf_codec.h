#pragma once

#include <functional>
#include <string>
#include <muduo/net/Buffer.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Timestamp.h>
#include "chat.pb.h"

namespace chat
{

    using muduo::Timestamp;
    using muduo::net::Buffer;
    using muduo::net::TcpConnectionPtr;

    class ProtobufCodec
    {
    public:
        using MessageCallback = std::function<void(const TcpConnectionPtr &, const chatroom::ChatMessage &, Timestamp)>;

        static const int kHeaderLen = 4;
        static const int kMaxMessageLen = 64 * 1024 * 1024;

        explicit ProtobufCodec(const MessageCallback &messageCallback);

        void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime);
        static void send(const TcpConnectionPtr &conn, const chatroom::ChatMessage &message);

    private:
        MessageCallback messageCallback_;
    };

} // namespace chat
