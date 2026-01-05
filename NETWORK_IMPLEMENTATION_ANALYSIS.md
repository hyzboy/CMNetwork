# CMNetwork 网络库实现分析

## 1. 概述

CMNetwork 是一个用 C++ 编写的跨平台网络通信库，提供了对多种网络协议和通信模式的封装。该库采用了面向对象的设计，支持 TCP、UDP、SCTP 和 WebSocket 等多种协议。

### 1.1 核心特性

- **跨平台支持**: 支持 Windows、Linux、macOS 和各种 BSD 系统
- **多协议支持**: TCP、UDP、UDP-Lite、SCTP、WebSocket
- **多线程架构**: 提供多线程服务器和客户端实现
- **事件驱动**: 使用 epoll（Linux）、kqueue（BSD/macOS）和 select（Windows）
- **流式 I/O**: 提供输入输出流接口，简化数据处理
- **连接管理**: 完善的连接生命周期管理和错误处理

## 2. 架构设计

### 2.1 模块组织

```
CMNetwork/
├── inc/hgl/network/          # 头文件
│   ├── IP.h                  # IP地址相关
│   ├── Socket.h              # Socket基类
│   ├── TCPSocket.h           # TCP Socket
│   ├── TCPClient.h           # TCP客户端
│   ├── TCPServer.h           # TCP服务器
│   ├── UdpSocket.h           # UDP Socket
│   ├── SCTPSocket.h          # SCTP Socket
│   ├── WebSocket.h           # WebSocket支持
│   ├── SocketManage.h        # Socket管理
│   ├── MultiThreadAccept.h   # 多线程接入
│   └── ...
└── src/                      # 源文件
    ├── Socket.cpp
    ├── TCPSocket.cpp
    ├── TCPClient.cpp
    ├── TCPServer.cpp
    ├── SocketManage.cpp
    ├── SocketManageEpoll.cpp    # Linux实现
    ├── SocketManageKqueue.cpp   # BSD/macOS实现
    ├── SocketManageSelect.cpp   # Windows实现
    └── ...
```

### 2.2 核心类层次结构

```
Socket (基类)
├── TCPSocket (TCP连接处理)
│   ├── TCPClient (TCP客户端)
│   └── IOSocket
│       └── TCPAccept (服务器端连接)
├── UdpSocket (UDP通信)
├── UdpLiteSocket (UDP-Lite通信)
└── SCTPSocket (SCTP通信)
    ├── SCTPO2OSocket (一对一)
    └── SCTPO2MSocket (一对多)

IPAddress (地址基类)
├── IPv4Address (IPv4地址)
└── IPv6Address (IPv6地址)

SocketManage (Socket管理)
└── SocketManageBase (平台特定实现)
    ├── SocketManageEpoll (Linux)
    ├── SocketManageKqueue (BSD/macOS)
    └── SocketManageSelect (Windows)
```

## 3. 核心组件详解

### 3.1 IP 地址管理 (IP.h, IPAddress.cpp)

#### 3.1.1 设计特点

- **抽象基类**: `IPAddress` 提供统一接口
- **双协议栈**: 支持 IPv4 和 IPv6
- **协议绑定**: 与 TCP/UDP/SCTP 等协议类型绑定
- **地址解析**: 支持域名解析和地址列表获取

#### 3.1.2 核心功能

```cpp
class IPAddress {
    virtual bool Set(const char *name, ushort port, int socktype, int protocol) = 0;
    virtual bool Bind(int socket, int reuse = 1) const = 0;
    virtual void ToString(char *, int) const = 0;
    virtual IPAddress *CreateCopy() const = 0;
};
```

**IPv4Address** 和 **IPv6Address** 分别实现：
- 地址设置和解析
- Socket 绑定
- 字符串转换
- 广播地址支持（仅IPv4）
- 域名和本机IP列表获取

#### 3.1.3 关键常量

```cpp
constexpr uint HGL_NETWORK_MAX_PORT = 65535;           // 最大端口号
constexpr uint HGL_NETWORK_TIME_OUT = HGL_TIME_ONE_MINUTE;  // 默认超时1分钟
constexpr uint HGL_NETWORK_HEART_TIME = HGL_NETWORK_TIME_OUT/2;  // 心跳时间30秒
constexpr uint HGL_TCP_BUFFER_SIZE = HGL_SIZE_1KB * 256;  // TCP缓冲区256KB
```

### 3.2 Socket 基类 (Socket.h, Socket.cpp)

#### 3.2.1 基本职责

- 封装底层 socket 文件描述符
- 提供跨平台的 socket 创建和销毁
- 管理 socket 地址信息
- 设置阻塞/非阻塞模式

#### 3.2.2 核心方法

```cpp
class Socket {
protected:
    IPAddress *ThisAddress;    // Socket地址
    int ThisSocket;            // Socket文件描述符
    
public:
    bool InitSocket(const IPAddress *);     // 创建Socket
    bool UseSocket(int, const IPAddress *); // 使用已存在的Socket
    bool ReCreateSocket();                  // 重新创建Socket
    void CloseSocket();                     // 关闭连接
    void SetBlock(bool block, double sto, double rto);  // 设置阻塞模式
};
```

#### 3.2.3 错误处理

定义了完善的错误枚举：
```cpp
enum SocketError {
    nseNoError = 0,           // 无错误
    nseInt = 4,               // 系统中断
    nseIOError = 5,           // I/O错误
    nseTooManyLink = 24,      // 连接过多
    nseBrokenPipe = 32,       // 管道破裂
    nseWouldBlock,            // 需要重试
    nseSoftwareBreak,         // 我方断开
    nsePeerBreak,             // 对方断开
    nseTimeOut                // 超时
};
```

### 3.3 TCP Socket (TCPSocket.h, TCPSocket.cpp)

#### 3.3.1 特性

- 继承自 `Socket` 基类
- 提供连接状态检测
- 支持 TCP_NODELAY（禁用 Nagle 算法）
- 支持 Keep-Alive 机制
- 提供等待接收功能

#### 3.3.2 核心功能

```cpp
class TCPSocket : public Socket {
protected:
    timeval time_out;
    fd_set local_set, recv_set, err_set;
    
public:
    bool SetNodelay(bool);                          // 无延迟模式
    void SetKeepAlive(bool, int, int, int);         // 保持连接
    bool IsConnect();                               // 检查连接状态
    int WaitRecv(double timeout);                   // 等待接收数据
};
```

### 3.4 TCP 客户端 (TCPClient.h, TCPClient.cpp)

#### 3.4.1 设计模式

- **多线程阻塞模式**: 独立的收包和发包线程
- **流式接口**: 提供 InputStream 和 OutputStream
- **心跳支持**: 可配置的心跳间隔
- **超时控制**: 可配置的超时时间

#### 3.4.2 核心接口

```cpp
class TCPClient : public TCPSocket {
private:
    io::InputStream *sis;      // 输入流
    io::OutputStream *sos;     // 输出流
    char *ipstr;               // IP字符串
    
public:
    double Heart;              // 心跳间隔(默认30秒)
    double TimeOut;            // 超时时间(默认60秒)
    
    bool Connect();                           // 连接服务器
    bool CreateConnect(const IPAddress *);    // 创建连接
    void Disconnect();                        // 断开连接
    
    io::InputStream *GetInputStream();        // 获取输入流
    io::OutputStream *GetOutputStream();      // 获取输出流
};
```

### 3.5 TCP 服务器 (TCPServer.h, AcceptServer.h)

#### 3.5.1 架构

```
AcceptServer (服务器基类)
└── TCPServer (TCP服务器实现)

ServerSocket (监听Socket)
└── 管理监听端口和接受连接

TCPAccept (连接处理)
└── 处理单个客户端连接

MultiThreadAccept (多线程接入)
└── 管理多个接入线程
```

#### 3.5.2 核心特性

- **多线程接入**: 支持多个线程同时接受连接
- **连接管理**: 统一管理所有客户端连接
- **事件驱动**: 基于 epoll/kqueue/select 的事件循环
- **负载控制**: 服务器超载保护和恢复机制

### 3.6 Socket 管理 (SocketManage.h, SocketManage.cpp)

#### 3.6.1 设计理念

`SocketManage` 是一个非线程安全的 Socket 管理类，负责：
- 维护所有活跃连接
- 轮询 Socket 事件
- 分发接收、发送和错误事件
- 管理错误连接

#### 3.6.2 核心方法

```cpp
class SocketManage {
protected:
    Map<int, TCPAccept *> socket_list;      // Socket列表
    SocketManageBase *manage;               // 平台特定实现
    
    SocketEventList sock_recv_list;         // 接收事件列表
    SocketEventList sock_send_list;         // 发送事件列表
    SocketEventList sock_error_list;        // 错误事件列表
    
    TCPAcceptSet error_sets;                // 错误Socket集合
    
public:
    bool Join(TCPAccept *s);                // 加入管理
    bool Unjoin(TCPAccept *s);              // 移除管理
    int Update(const double &time_out);     // 更新所有操作
    const TCPAcceptSet &GetErrorSocketSet(); // 获取错误集合
};
```

#### 3.6.3 平台特定实现

**Linux - SocketManageEpoll**
- 使用 epoll 实现高性能事件通知
- 边缘触发模式（EPOLLET）
- 支持大量并发连接

```cpp
class SocketManageEpoll : public SocketManageBase {
    int epoll_fd;              // epoll文件描述符
    epoll_event *event_list;   // 事件列表
    
    bool epoll_add(int sock);  // 添加到epoll
    bool epoll_del(int sock);  // 从epoll删除
};
```

**BSD/macOS - SocketManageKqueue**
- 使用 kqueue 实现事件通知
- 支持 EVFILT_READ 和 EVFILT_WRITE 过滤器

**Windows - SocketManageSelect**
- 使用 select 实现事件通知
- 兼容性最好但性能相对较低

### 3.7 UDP Socket (UdpSocket.h, UdpSocket.cpp)

#### 3.7.1 特性

- 支持单播、广播和多播
- 提供原始数据收发接口
- 支持 UDP-Lite 协议（可选）
- 地址重用和端口复用

#### 3.7.2 核心方法

```cpp
class UdpSocket : public Socket {
public:
    virtual int SendTo(const void *, int, const IPAddress *);    // 发送到指定地址
    virtual int RecvFrom(void *, int, IPAddress *);               // 从指定地址接收
};
```

### 3.8 SCTP Socket (SCTPSocket.h, SCTPSocket.cpp)

#### 3.8.1 SCTP 支持

SCTP（Stream Control Transmission Protocol）是一个面向消息的可靠传输协议，结合了 TCP 和 UDP 的优点。

#### 3.8.2 实现模式

**SCTPO2OSocket (一对一)**
```cpp
class SCTPO2OSocket : public SCTPSocket {
    bool SendMsg(const void *, int len, uint16 stream);     // 发送消息
    bool RecvMsg(MemBlock<char> *, uint16 &stream);         // 接收消息
};
```

**SCTPO2MSocket (一对多)**
```cpp
class SCTPO2MSocket : public SCTPSocket {
    bool SendMsg(const sockaddr_in *, void *, int len, uint16);      // 发送到指定地址
    bool RecvMsg(sockaddr_in &, void *, int max_len, int &len, uint16 &); // 接收消息
};
```

### 3.9 WebSocket 支持 (WebSocket.h, WebSocketAccept.cpp)

#### 3.9.1 功能

- WebSocket 握手处理
- Sec-WebSocket-Key 验证
- Sec-WebSocket-Accept 生成
- 协议版本检查

#### 3.9.2 核心函数

```cpp
bool GetWebSocketInfo(
    U8String &sec_websocket_key,
    U8String &sec_websocket_protocol,
    uint &sec_websocket_version,
    const u8char *data,
    const uint size
);

void MakeWebSocketAccept(
    U8String &result,
    const U8String &sec_websocket_key,
    const U8String &sec_websocket_protocol
);
```

### 3.10 流式 I/O (SocketInputStream.h, SocketOutputStream.h)

#### 3.10.1 设计目的

- 提供类似 Java 的流式接口
- 简化数据读写操作
- 支持缓冲和类型转换
- 统一的错误处理

#### 3.10.2 核心类

**SocketInputStream** - 封装 Socket 接收操作
**SocketOutputStream** - 封装 Socket 发送操作

这些类继承自 `io::DataInputStream` 和 `io::DataOutputStream`，提供了丰富的数据类型读写方法。

### 3.11 HTTP 支持 (HTTPInputStream.h, HTTPInputStream.cpp)

#### 3.11.1 功能

- HTTP 头解析
- HTTP 请求/响应处理
- Content-Length 处理
- 分块传输编码支持

## 4. 关键技术实现

### 4.1 跨平台抽象

#### 4.1.1 Socket 创建

```cpp
#if HGL_OS == HGL_OS_Windows
    // Windows下初始化WinSocket
    bool InitWinSocket() {
        WSADATA wsa;
        return (WSAStartup(MAKEWORD(2,2), &wsa) == NO_ERROR);
    }
#endif
```

#### 4.1.2 错误码统一

```cpp
#if HGL_OS == HGL_OS_Windows
    #define GetLastSocketError() WSAGetLastError()
    nseWouldBlock = WSAEWOULDBLOCK,
#else
    #define GetLastSocketError() (errno)
    nseWouldBlock = EWOULDBLOCK,
#endif
```

### 4.2 非阻塞 I/O

#### 4.2.1 设置非阻塞模式

```cpp
void SetSocketBlock(int sock, bool block, double send_timeout, double recv_timeout) {
    #if HGL_OS == HGL_OS_Windows
        u_long mode = block ? 0 : 1;
        ioctlsocket(sock, FIONBIO, &mode);
    #else
        int flags = fcntl(sock, F_GETFL, 0);
        if (block)
            flags &= ~O_NONBLOCK;
        else
            flags |= O_NONBLOCK;
        fcntl(sock, F_SETFL, flags);
    #endif
    
    // 设置超时
    struct timeval tv;
    tv.tv_sec = (int)send_timeout;
    tv.tv_usec = (send_timeout - tv.tv_sec) * 1000000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    // ... 设置接收超时
}
```

### 4.3 事件驱动模型

#### 4.3.1 Epoll (Linux)

```cpp
// 边缘触发模式，高性能
ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLRDHUP | EPOLLHUP;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

// 等待事件
int count = epoll_wait(epoll_fd, event_list, max_connect, timeout_ms);
```

#### 4.3.2 Kqueue (BSD/macOS)

```cpp
struct kevent change_list[2];
EV_SET(&change_list[0], sock, EVFILT_READ, EV_ADD, 0, 0, nullptr);
EV_SET(&change_list[1], sock, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
kevent(kqueue_fd, change_list, 2, nullptr, 0, nullptr);
```

#### 4.3.3 Select (Windows)

```cpp
fd_set read_set, write_set, error_set;
FD_ZERO(&read_set);
FD_ZERO(&write_set);
FD_ZERO(&error_set);

// 添加所有socket
for (auto &pair : socket_list) {
    FD_SET(pair.first, &read_set);
    FD_SET(pair.first, &write_set);
    FD_SET(pair.first, &error_set);
}

select(max_fd + 1, &read_set, &write_set, &error_set, &timeout);
```

### 4.4 多线程接入

#### 4.4.1 接入线程

```cpp
class AcceptThread : public Thread {
    bool Execute() override {
        while (!IsClose()) {
            // 等待新连接
            int client_sock = accept(server_socket, ...);
            if (client_sock > 0) {
                IPAddress *addr = ...; // 获取客户端地址
                OnAccept(client_sock, addr);
            }
        }
    }
    
    virtual bool OnAccept(int sock, IPAddress *addr) = 0;
};
```

#### 4.4.2 多线程管理

```cpp
template<typename ACCEPT_THREAD>
class MultiThreadAccept {
    MultiThreadManage<ACCEPT_THREAD> accept_thread_manage;
    Semaphore active_semaphore;
    
    bool Init(AcceptServer *server, uint thread_count) {
        for (int i = 0; i < thread_count; i++) {
            accept_thread_manage.Add(new ACCEPT_THREAD(server, &active_semaphore));
        }
    }
};
```

### 4.5 连接保持和心跳

#### 4.5.1 TCP Keep-Alive

```cpp
void TCPSocket::SetKeepAlive(bool enable, int idle_time, int interval, int count) {
    int optval = enable ? 1 : 0;
    setsockopt(ThisSocket, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    
    #ifdef TCP_KEEPIDLE
        setsockopt(ThisSocket, IPPROTO_TCP, TCP_KEEPIDLE, &idle_time, sizeof(idle_time));
    #endif
    #ifdef TCP_KEEPINTVL
        setsockopt(ThisSocket, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    #endif
    #ifdef TCP_KEEPCNT
        setsockopt(ThisSocket, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    #endif
}
```

#### 4.5.2 应用层心跳

```cpp
class TCPClient {
    double Heart;       // 心跳间隔时间(默认30秒)
    double TimeOut;     // 超时时间(默认60秒)
    
    // 心跳并不是每隔指定时间都发送
    // 而是离上一次发送任意封包超过指定时间才发送
};
```

### 4.6 缓冲区管理

#### 4.6.1 缓冲区大小

```cpp
constexpr uint HGL_TCP_BUFFER_SIZE = HGL_SIZE_1KB * 256;  // 256KB

// 设置发送和接收缓冲区
int buf_size = HGL_TCP_BUFFER_SIZE;
setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
```

### 4.7 TCP_NODELAY

```cpp
bool TCPSocket::SetNodelay(bool enable) {
    int flag = enable ? 1 : 0;
    return setsockopt(ThisSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == 0;
}
```

禁用 Nagle 算法可以减少小包的延迟，适用于实时性要求高的应用。

## 5. 数据流向

### 5.1 TCP 客户端数据流

```
应用层
  ↓ (写入数据)
OutputStream
  ↓ (缓冲)
SocketOutputStream
  ↓ (send/write)
内核 TCP 发送缓冲区
  ↓ (网络传输)
服务器
```

```
服务器
  ↓ (网络接收)
内核 TCP 接收缓冲区
  ↓ (recv/read)
SocketInputStream
  ↓ (缓冲)
InputStream
  ↓ (读取数据)
应用层
```

### 5.2 TCP 服务器连接流程

```
ServerSocket (监听)
  ↓ (accept)
AcceptThread (接入线程)
  ↓ (创建连接对象)
TCPAccept (连接对象)
  ↓ (加入管理)
SocketManage (Socket管理器)
  ↓ (事件循环)
[接收事件] → ProcRecv
[发送事件] → ProcSend
[错误事件] → ProcError
```

### 5.3 SocketManage 事件循环

```
Update() 开始
  ↓
清除上次的错误Socket
  ↓
调用平台特定的 Update (epoll_wait/kevent/select)
  ↓
获取事件列表
  ↓
分类事件
  ├─ 接收事件 → sock_recv_list
  ├─ 发送事件 → sock_send_list
  └─ 错误事件 → sock_error_list
  ↓
处理接收事件 (ProcSocketRecvList)
  ↓
处理发送事件 (ProcSocketSendList)
  ↓
处理错误事件 (ProcSocketErrorList)
  ↓
返回处理的Socket数量
```

## 6. 使用场景和最佳实践

### 6.1 TCP 客户端使用

```cpp
// 创建IPv4 TCP客户端
IPv4Address *addr = CreateIPv4TCP("example.com", 80);
TCPClient *client = new TCPClient();

// 连接服务器
if (client->CreateConnect(addr)) {
    // 设置心跳和超时
    client->Heart = 30.0;    // 30秒心跳
    client->TimeOut = 60.0;  // 60秒超时
    
    // 发送数据
    auto *out = client->GetOutputStream();
    out->WriteUTF8String("Hello Server");
    
    // 接收数据
    auto *in = client->GetInputStream();
    U8String response;
    in->ReadUTF8String(response);
    
    // 断开连接
    client->Disconnect();
}

delete client;
delete addr;
```

### 6.2 TCP 服务器使用

```cpp
// 创建服务器
class MyTCPAccept : public TCPAccept {
    void ProcRecv() override {
        // 处理接收到的数据
    }
};

class MyAcceptThread : public AcceptThread {
    bool OnAccept(int sock, IPAddress *addr) override {
        // 创建连接对象
        auto *accept = new MyTCPAccept();
        accept->UseSocket(sock, addr);
        // 加入管理器
        socket_manage->Join(accept);
        return true;
    }
};

// 初始化服务器
TCPServer server;
IPv4Address *addr = CreateIPv4TCP(8080);
server.CreateServerSocket(addr);

// 启动多线程接入
MultiThreadAccept<MyAcceptThread> mta;
mta.Init(&server, 4);  // 4个接入线程
mta.Start();

// 事件循环
SocketManage socket_manage(1000);
while (running) {
    socket_manage.Update(5.0);  // 5秒超时
    
    // 处理错误连接
    auto &error_set = socket_manage.GetErrorSocketSet();
    for (auto *conn : error_set) {
        socket_manage.Unjoin(conn);
        delete conn;
    }
}
```

### 6.3 UDP 使用

```cpp
// 创建UDP Socket
UdpSocket udp;
IPv4Address *addr = CreateIPv4UDP(9999);
udp.CreateReceiveSocket(addr);

// 发送数据
IPv4Address *dest = CreateIPv4UDP("192.168.1.100", 8888);
udp.SendTo(data, size, dest);

// 接收数据
IPv4Address *sender = new IPv4Address();
int received = udp.RecvFrom(buffer, buffer_size, sender);
if (received > 0) {
    // 处理数据
}
```

### 6.4 WebSocket 握手

```cpp
// 解析WebSocket握手
U8String sec_key, sec_protocol;
uint version;
GetWebSocketInfo(sec_key, sec_protocol, version, data, size);

// 生成Accept响应
U8String accept_response;
MakeWebSocketAccept(accept_response, sec_key, sec_protocol);

// 发送握手响应
// HTTP/1.1 101 Switching Protocols
// Upgrade: websocket
// Connection: Upgrade
// Sec-WebSocket-Accept: {accept_response}
```

## 7. 性能优化

### 7.1 零拷贝技术

```cpp
#if HGL_OS == HGL_OS_Linux
    // Linux sendfile
    int sendfile(int tfd, int sfd, size_t size);
#elif HGL_OS == HGL_OS_FreeBSD
    // FreeBSD sendfile
    int sendfile(int tfd, int sfd, off_t offset, size_t size, ...);
#endif
```

### 7.2 边缘触发模式 (Linux)

```cpp
ev.events = EPOLLIN | EPOLLOUT | EPOLLET;  // EPOLLET 边缘触发
```

边缘触发模式要求：
- 读/写时必须一直读/写直到出错（EAGAIN）
- 减少系统调用次数
- 提高并发性能

### 7.3 连接池和对象复用

```cpp
class AcceptThread {
    IPAddressStack ip_stack;  // IP地址对象池
    // 预分配IP地址对象，避免频繁new/delete
};
```

### 7.4 批量操作

```cpp
// 批量加入管理
int Join(TCPAccept **s_list, int count);

// 批量移除管理
int Unjoin(TCPAccept **s_list, int count);
```

## 8. 安全性考虑

### 8.1 缓冲区溢出防护

- 所有接收操作都有长度限制
- 使用安全的内存操作函数

### 8.2 超时保护

```cpp
constexpr uint HGL_NETWORK_TIME_OUT = HGL_TIME_ONE_MINUTE;       // 60秒
constexpr uint HGL_NETWORK_DOUBLE_TIME_OUT = HGL_NETWORK_TIME_OUT * 2;  // 120秒
```

### 8.3 连接验证

- 支持 SSL/TLS（需要额外实现）
- WebSocket 握手验证
- 地址验证和过滤

### 8.4 资源限制

```cpp
constexpr uint HGL_SERVER_LISTEN_COUNT = SOMAXCONN;  // 最大监听队列
constexpr uint HGL_SERVER_OVERLOAD_RESUME_TIME = 10; // 超载恢复等待时间
```

## 9. 错误处理和日志

### 9.1 日志系统

```cpp
#define OBJECT_LOGGER  // 对象级别日志宏

GLogInfo("Create TCP Socket OK: " + socket_id);
GLogError("CreateSocket failed; errno " + error_code);
LOG_INFO("SocketManageEpoll::Join() Socket:" + socket_id);
LOG_ERROR("epoll_add failed for socket:" + socket_id);
```

### 9.2 错误码处理

```cpp
const int sock_error = GetLastSocketError();
const os_char *error_string = GetLastSocketErrorString();
```

### 9.3 断线检测

```cpp
enum SocketError {
    nseSoftwareBreak,  // 我方软件主动断开
    nsePeerBreak,      // 对方主动断开
    nseBrokenPipe,     // 管道破裂
};
```

## 10. 扩展性和可维护性

### 10.1 模块化设计

- **传输层**: UDP、TCP、SCTP 独立实现
- **应用层**: HTTP、WebSocket 基于传输层构建
- **平台层**: 平台特定代码隔离

### 10.2 接口抽象

```cpp
// 基类提供虚接口
class SocketManageBase {
    virtual bool Join(int sock) = 0;
    virtual bool Unjoin(int sock) = 0;
    virtual int Update(const double &time_out) = 0;
};

// 平台特定实现
class SocketManageEpoll : public SocketManageBase { ... };
class SocketManageKqueue : public SocketManageBase { ... };
class SocketManageSelect : public SocketManageBase { ... };
```

### 10.3 配置管理

```cmake
# CMakeLists.txt
SET(BUILD_NETWORK_UDP_LITE OFF)  # 可选UDP-Lite支持
SET(BUILD_NETWORK_SCTP OFF)      # 可选SCTP支持
```

### 10.4 调试支持

```cpp
#define HGL_RECV_BYTE_COUNT  // 接收字节数统计(调试用)
#define HGL_SEND_BYTE_COUNT  // 发送字节数统计(调试用)
#define HGL_SOCKET_SEND_LIMIT_SIZE  // 发送限制包尺寸
```

## 11. 优势与特点

### 11.1 跨平台兼容性

- 完整的 Windows、Linux、macOS、BSD 支持
- 自动选择最优的事件通知机制
- 统一的 API 接口

### 11.2 高性能

- 边缘触发的 epoll（Linux）
- 高效的 kqueue（BSD/macOS）
- 非阻塞 I/O
- 零拷贝技术支持

### 11.3 易用性

- 面向对象的设计
- 流式 I/O 接口
- 丰富的辅助函数
- 完善的示例和文档

### 11.4 可靠性

- 完善的错误处理
- 连接状态管理
- 心跳和超时机制
- 资源自动清理

### 11.5 可扩展性

- 模块化架构
- 虚函数接口
- 支持自定义协议
- 插件式的协议支持

## 12. 潜在改进方向

### 12.1 异步 I/O 支持

- 考虑添加完全异步的 API
- 支持协程（C++20）
- 提供回调接口

### 12.2 SSL/TLS 集成

- 添加安全通信支持
- 证书管理
- 加密协议协商

### 12.3 HTTP/2 和 HTTP/3

- 升级 HTTP 支持到 HTTP/2
- QUIC 协议支持（HTTP/3）

### 12.4 连接池

- 客户端连接池
- 连接复用
- 自动重连

### 12.5 性能监控

- 更详细的统计信息
- 性能指标收集
- 实时监控接口

### 12.6 内存管理

- 对象池
- 自定义分配器
- 智能指针（unique_ptr/shared_ptr）

## 13. 总结

CMNetwork 是一个设计良好、功能完善的网络通信库，具有以下突出特点：

1. **架构清晰**: 分层设计，职责明确，易于理解和维护
2. **跨平台**: 完整支持主流操作系统，自动适配最优实现
3. **高性能**: 采用现代事件驱动架构，支持大量并发连接
4. **易用性**: 面向对象接口，流式 I/O，降低使用难度
5. **可靠性**: 完善的错误处理和连接管理机制
6. **扩展性**: 模块化设计，易于添加新协议和功能

该库适用于：
- 高性能服务器开发
- 实时通信应用
- 游戏服务器
- IoT 设备通信
- 分布式系统

对于需要构建网络应用的 C++ 开发者来说，这是一个值得考虑的基础库。其成熟的设计和完善的功能可以显著提高开发效率，同时保证系统的性能和可靠性。
