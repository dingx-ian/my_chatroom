#include "protobuf_codec.h"
#include <arpa/inet.h>
#include <muduo/base/Logging.h>

namespace chat
{

    ProtobufCodec::ProtobufCodec(const MessageCallback &messageCallback)
        : messageCallback_(messageCallback)
    {
    }

    void ProtobufCodec::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime)
    {
        while (buf->readableBytes() >= kHeaderLen)
        {
            int32_t be32 = 0;
            ::memcpy(&be32, buf->peek(), sizeof(be32));
            int32_t len = ::ntohl(be32);
            if (len <= 0 || len > kMaxMessageLen)
            {
                LOG_ERROR << "Invalid message length: " << len;
                conn->shutdown();
                return;
            }

            if (buf->readableBytes() < kHeaderLen + len)
            {
                break;
            }

            buf->retrieve(kHeaderLen);
            std::string payload(buf->peek(), len);
            chatroom::ChatMessage message;
            if (!message.ParseFromString(payload))
            {
                LOG_ERROR << "Failed to parse ChatMessage";
                conn->shutdown();
                return;
            }
            buf->retrieve(len);
            messageCallback_(conn, message, receiveTime);
        }
    }

    void ProtobufCodec::send(const TcpConnectionPtr &conn, const chatroom::ChatMessage &message)
    {
        std::string payload;
        if (!message.SerializeToString(&payload))
        {
            LOG_ERROR << "Failed to serialize ChatMessage";
            return;
        }

        int32_t len = static_cast<int32_t>(payload.size());
        int32_t be32 = ::htonl(len);
        conn->send(&be32, sizeof(be32));
        conn->send(payload);
    }

} // namespace chat
