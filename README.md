# my_chatroom

基于 muduo 的 C++ 高性能集群聊天室（项目第1周：骨架与单机原型）

## 技术栈

- C++11
- muduo 网络库
- Protobuf
- Redis / MySQL / Nginx（后续集群扩展）
- CMake

## 项目结构

```
my_chatroom/
├── proto/           # Protobuf 协议定义
├── src/             # C++ 源代码
├── build/           # 编译输出目录
├── CMakeLists.txt   # 构建配置
```

## 当前进度

- 已完成基础 Echo 服务器原型
- 已添加 Protobuf 协议定义
- 已支持 TCP 粘包处理（长度前缀编解码）
- 已实现用户连接管理和消息广播、私聊基础

## 构建与运行

### 依赖

- CMake 3.10+
- protobuf
- muduo
- pthread

### 编译

```bash
mkdir -p build
cd build
cmake ..
make
```

### 运行

```bash
./bin/chat_server
```

## 目录说明

- `proto/chat.proto`：聊天消息协议定义
- `src/protobuf_codec.*`：长度前缀 + Protobuf 编解码器
- `src/chat_service.*`：在线用户管理、广播与私聊逻辑
- `src/server.cc`：Muduo TCP 服务器入口
