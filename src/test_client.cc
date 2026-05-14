#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <iostream>
#include <string>
#include "chat.pb.h"
#include "protobuf_codec.h"

using namespace muduo;
using namespace muduo::net;

class TestClient
{
public:
    TestClient(EventLoop *loop, const InetAddress &serverAddr)
        : client_(loop, serverAddr, "TestClient")
    {
        client_.setConnectionCallback(
            std::bind(&TestClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&TestClient::onMessage, this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
    }

    void connect() { client_.connect(); }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "Connected to server";

            // 发送一条登录消息
            chatroom::ChatMessage msg;
            msg.set_type(chatroom::LOGIN);
            msg.set_from("test_user");
            msg.set_content("Hello, I'm test user!");
            msg.set_timestamp(Timestamp::now().microSecondsSinceEpoch());

            ProtobufCodec::send(conn, msg);
            LOG_INFO << "Sent login message";
        }
        else
        {
            LOG_INFO << "Disconnected from server";
        }
    }

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp time)
    {
        // 收到消息（目前只是打印，不做处理）
        LOG_INFO << "Received " << buf->readableBytes() << " bytes";
    }

    TcpClient client_;
};

int main()
{
    Logger::setLogLevel(Logger::INFO);

    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8888);

    TestClient client(&loop, serverAddr);
    client.connect();

    // 连接后 3 秒自动退出
    loop.runAfter(3.0, [&loop]()
                  {
        LOG_INFO << "Test done, quitting...";
        loop.quit(); });

    loop.loop();
    return 0;
}
