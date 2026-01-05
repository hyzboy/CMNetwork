"# CMNetwork

一个跨平台的 C++ 网络通信库 / A Cross-Platform C++ Network Communication Library

## 简介 / Introduction

CMNetwork 是一个功能完善、高性能的 C++ 网络通信库，提供了对多种网络协议的封装，支持 Windows、Linux、macOS 和各种 BSD 系统。

CMNetwork is a comprehensive, high-performance C++ network communication library that provides abstractions for multiple network protocols, supporting Windows, Linux, macOS, and various BSD systems.

## 核心特性 / Key Features

- 🌐 **多协议支持** / Multi-Protocol Support
  - TCP, UDP, UDP-Lite, SCTP, WebSocket, HTTP

- 💻 **跨平台** / Cross-Platform
  - Windows (select), Linux (epoll), macOS/BSD (kqueue)

- ⚡ **高性能** / High Performance
  - 事件驱动 / Event-driven architecture
  - 边缘触发模式 / Edge-triggered mode (Linux)
  - 非阻塞 I/O / Non-blocking I/O

- 🧵 **多线程** / Multi-Threading
  - 多线程服务器 / Multi-threaded server
  - 线程池支持 / Thread pool support

- 📦 **易用接口** / Easy-to-Use Interface
  - 面向对象设计 / Object-oriented design
  - 流式 I/O 接口 / Stream-based I/O

## 架构 / Architecture

```
Socket (基类/Base)
├── TCPSocket (TCP连接/TCP Connection)
│   ├── TCPClient (客户端/Client)
│   └── TCPAccept (服务器连接/Server Connection)
├── UdpSocket (UDP通信/UDP Communication)
└── SCTPSocket (SCTP通信/SCTP Communication)

SocketManage (Socket管理器/Socket Manager)
├── SocketManageEpoll (Linux优化/Linux Optimized)
├── SocketManageKqueue (BSD/macOS优化/BSD/macOS Optimized)
└── SocketManageSelect (Windows兼容/Windows Compatible)
```

## 文档 / Documentation

- 📖 [完整实现分析 / Full Implementation Analysis](NETWORK_IMPLEMENTATION_ANALYSIS.md) (中文)
- 📋 [架构概览 / Architecture Overview](ARCHITECTURE_OVERVIEW.md) (English)
- 📝 [分析总结 / Analysis Summary](ANALYSIS_SUMMARY.md) (中文)

## 构建 / Build

```bash
mkdir build
cd build
cmake ..
make
```

## 使用示例 / Usage Example

### TCP 客户端 / TCP Client

```cpp
#include <hgl/network/TCPClient.h>

IPv4Address *addr = CreateIPv4TCP("example.com", 8080);
TCPClient *client = new TCPClient();

if (client->CreateConnect(addr)) {
    auto *out = client->GetOutputStream();
    out->WriteUTF8String("Hello Server");
    
    auto *in = client->GetInputStream();
    U8String response;
    in->ReadUTF8String(response);
    
    client->Disconnect();
}
```

### TCP 服务器 / TCP Server

```cpp
#include <hgl/network/TCPServer.h>
#include <hgl/network/SocketManage.h>

TCPServer server;
server.CreateServerSocket(CreateIPv4TCP(8080));

SocketManage socket_manage(1000);
while (running) {
    socket_manage.Update(5.0);
    // 处理连接 / Handle connections
}
```

## 许可证 / License

请查看 LICENSE 文件 / Please see the LICENSE file

## 贡献 / Contributing

欢迎提交 Pull Request 和 Issue / Pull requests and issues are welcome!" 
