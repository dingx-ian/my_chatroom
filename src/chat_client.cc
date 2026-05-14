#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include "protobuf_codec.h"
#include "chat.pb.h"

using namespace muduo;
using namespace muduo::net;

class ChatClient
{
public:
    ChatClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &username)
        : loop_(loop),
          client_(loop, serverAddr, "ChatClient"),
          username_(username),
          codec_(std::bind(&ChatClient::onMessage, this,
                           std::placeholders::_1,
                           std::placeholders::_2,
                           std::placeholders::_3)),
          connected_(false),
          exiting_(false)
    {
        client_.setConnectionCallback(
            std::bind(&ChatClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&chat::ProtobufCodec::onMessage, &codec_,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
    }

    void connect()
    {
        client_.connect();
    }

    ~ChatClient()
    {
        exiting_ = true;
        cond_.notify_one();
    }

    void runConsole()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this]
                       { return connected_ || exiting_; });
        }

        {
            std::lock_guard<std::mutex> coutLock(coutMutex_);
            std::cout << "登录成功，欢迎 [" << username_ << "]！" << std::endl;
        }

        while (!exiting_)
        {
            printPendingMessages();

            {
                std::lock_guard<std::mutex> coutLock(coutMutex_);
                std::cout << "=== 聊天室命令 ===" << std::endl;
                std::cout << "1. 公聊消息" << std::endl;
                std::cout << "2. 私聊消息" << std::endl;
                std::cout << "3. 退出" << std::endl;
                std::cout << "请输入选项: ";
            }

            std::string option;
            if (!std::getline(std::cin, option))
            {
                break;
            }

            if (option == "1")
            {
                sendPublicChat();
            }
            else if (option == "2")
            {
                sendPrivateChat();
            }
            else if (option == "3")
            {
                sendLogout();
                break;
            }
            else
            {
                std::lock_guard<std::mutex> coutLock(coutMutex_);
                std::cout << "无效选项，请重新输入。" << std::endl;
            }
        }

        loop_->quit();
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "Connected to server";
            {
                std::lock_guard<std::mutex> lock(mutex_);
                conn_ = conn;
                connected_ = true;
            }
            cond_.notify_one();

            chatroom::ChatMessage loginMsg;
            loginMsg.set_type(chatroom::LOGIN);
            loginMsg.set_from(username_);
            loginMsg.set_content(username_ + " has joined");
            loginMsg.set_timestamp(Timestamp::now().microSecondsSinceEpoch());
            sendInLoop(conn, loginMsg);
        }
        else
        {
            LOG_INFO << "Disconnected from server";
            std::lock_guard<std::mutex> lock(mutex_);
            conn_.reset();
            connected_ = false;
        }
    }

    void onMessage(const TcpConnectionPtr &, const chatroom::ChatMessage &message, Timestamp)
    {
        std::string text;
        switch (message.type())
        {
        case chatroom::CHAT_ALL:
            text = "[公聊] " + message.from() + ": " + message.content();
            break;
        case chatroom::CHAT_PRIVATE:
            text = "[私聊] " + message.from() + ": " + message.content();
            break;
        case chatroom::LOGIN:
        case chatroom::LOGOUT:
            text = "[系统] " + message.content();
            break;
        default:
            text = "[系统] 未知消息: " + message.content();
            break;
        }
        {
            std::lock_guard<std::mutex> lock(messageMutex_);
            pendingMessages_.push_back(text);
        }
    }

    void printPendingMessages()
    {
        std::deque<std::string> messages;
        {
            std::lock_guard<std::mutex> lock(messageMutex_);
            messages.swap(pendingMessages_);
        }

        std::lock_guard<std::mutex> coutLock(coutMutex_);
        for (const auto &msg : messages)
        {
            std::cout << msg << std::endl;
        }
    }

    void sendPublicChat()
    {
        std::cout << "请输入公聊内容: ";
        std::string content;
        if (!std::getline(std::cin, content) || content.empty())
        {
            std::cout << "消息不能为空" << std::endl;
            return;
        }

        chatroom::ChatMessage msg;
        msg.set_type(chatroom::CHAT_ALL);
        msg.set_from(username_);
        msg.set_content(content);
        msg.set_timestamp(Timestamp::now().microSecondsSinceEpoch());
        sendCurrentConnection(msg);
    }

    void sendPrivateChat()
    {
        std::cout << "请输入目标用户名: ";
        std::string target;
        if (!std::getline(std::cin, target) || target.empty())
        {
            std::cout << "目标用户名不能为空" << std::endl;
            return;
        }

        std::cout << "请输入私聊内容: ";
        std::string content;
        if (!std::getline(std::cin, content) || content.empty())
        {
            std::cout << "消息不能为空" << std::endl;
            return;
        }

        chatroom::ChatMessage msg;
        msg.set_type(chatroom::CHAT_PRIVATE);
        msg.set_from(username_);
        msg.set_to(target);
        msg.set_content(content);
        msg.set_timestamp(Timestamp::now().microSecondsSinceEpoch());
        sendCurrentConnection(msg);
    }

    void sendLogout()
    {
        {
            std::lock_guard<std::mutex> coutLock(coutMutex_);
            std::cout << "正在退出..." << std::endl;
        }

        chatroom::ChatMessage msg;
        msg.set_type(chatroom::LOGOUT);
        msg.set_from(username_);
        msg.set_content(username_ + " has left");
        msg.set_timestamp(Timestamp::now().microSecondsSinceEpoch());

        TcpConnectionPtr conn;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            conn = conn_;
        }
        if (conn)
        {
            auto loop = conn->getLoop();
            loop->runInLoop([this, conn, msg]()
                            {
                chat::ProtobufCodec::send(conn, msg);
                conn->shutdown();
                std::lock_guard<std::mutex> lock(mutex_);
                if (conn_ == conn)
                {
                    conn_.reset();
                    connected_ = false;
                } });
        }
        exiting_ = true;
    }

    void sendCurrentConnection(const chatroom::ChatMessage &message)
    {
        TcpConnectionPtr conn;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            conn = conn_;
        }
        if (!conn)
        {
            std::lock_guard<std::mutex> coutLock(coutMutex_);
            std::cout << "当前未连接到服务器，请稍后重试。" << std::endl;
            return;
        }
        sendInLoop(conn, message);
    }

    void sendInLoop(const TcpConnectionPtr &conn, const chatroom::ChatMessage &message)
    {
        conn->getLoop()->runInLoop([conn, message]()
                                   { chat::ProtobufCodec::send(conn, message); });
    }

    EventLoop *loop_;
    TcpClient client_;
    std::string username_;
    chat::ProtobufCodec codec_;
    std::mutex mutex_;
    std::mutex coutMutex_;
    std::mutex messageMutex_;
    std::condition_variable cond_;
    std::deque<std::string> pendingMessages_;
    TcpConnectionPtr conn_;
    bool connected_;
    std::atomic<bool> exiting_;
};

int main()
{
    Logger::setLogLevel(Logger::INFO);

    std::string serverIp;
    std::string portText;
    std::string username;

    std::cout << "服务器 IP (默认 127.0.0.1): ";
    std::getline(std::cin, serverIp);
    if (serverIp.empty())
    {
        serverIp = "127.0.0.1";
    }

    std::cout << "服务器端口 (默认 8888): ";
    std::getline(std::cin, portText);
    int serverPort = 8888;
    if (!portText.empty())
    {
        try
        {
            serverPort = std::stoi(portText);
        }
        catch (...)
        {
            serverPort = 8888;
        }
    }

    do
    {
        std::cout << "请输入用户名: ";
        std::getline(std::cin, username);
    } while (username.empty());

    EventLoop loop;
    InetAddress serverAddr(serverIp, static_cast<uint16_t>(serverPort));
    ChatClient client(&loop, serverAddr, username);
    client.connect();

    std::thread consoleThread([&client]()
                              { client.runConsole(); });

    loop.loop();

    if (consoleThread.joinable())
    {
        consoleThread.join();
    }

    return 0;
}
