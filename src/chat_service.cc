#include "chat_service.h"
#include "protobuf_codec.h"

namespace chat
{

    ChatService::ChatService() = default;

    void ChatService::onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "New client connected: " << conn->peerAddress().toIpPort();
        }
        else
        {
            LOG_INFO << "Client disconnected: " << conn->peerAddress().toIpPort();
            removeUser(conn);
        }
    }

    void ChatService::onMessage(const TcpConnectionPtr &conn, const chatroom::ChatMessage &message, Timestamp receiveTime)
    {
        switch (message.type())
        {
        case chatroom::LOGIN:
            addUser(message.from(), conn);
            break;

        case chatroom::LOGOUT:
            removeUser(conn);
            break;

        case chatroom::CHAT_ALL:
            broadcastMessage(message);
            break;

        case chatroom::CHAT_PRIVATE:
            sendPrivateMessage(message.to(), message);
            break;

        default:
            LOG_WARN << "Unsupported message type: " << message.type();
            break;
        }
    }

    void ChatService::addUser(const std::string &user, const TcpConnectionPtr &conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        userConnections_[user] = conn;
        LOG_INFO << "User logged in: " << user;
    }

    void ChatService::removeUser(const TcpConnectionPtr &conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = userConnections_.begin(); it != userConnections_.end(); ++it)
        {
            if (it->second == conn)
            {
                LOG_INFO << "Remove user: " << it->first;
                userConnections_.erase(it);
                break;
            }
        }
    }

    void ChatService::broadcastMessage(const chatroom::ChatMessage &message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &pair : userConnections_)
        {
            if (pair.second && pair.second->connected())
            {
                ProtobufCodec::send(pair.second, message);
            }
        }
    }

    void ChatService::sendPrivateMessage(const std::string &to, const chatroom::ChatMessage &message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userConnections_.find(to);
        if (it != userConnections_.end() && it->second && it->second->connected())
        {
            ProtobufCodec::send(it->second, message);
        }
        else
        {
            LOG_WARN << "Private message recipient not online: " << to;
        }
    }

} // namespace chat
