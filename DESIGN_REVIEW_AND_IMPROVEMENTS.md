# CMNetwork 设计审查与改进建议

## 概述

本文档基于对 CMNetwork 网络库的深入分析，提出设计改进建议、命名规范建议、扩展特性建议以及潜在的架构优化方案。

---

## 1. 设计问题与改进建议

### 1.1 内存管理问题

#### 🔴 问题：原始指针管理容易导致内存泄漏

**当前实现**：
```cpp
// Socket.h
class Socket {
protected:
    IPAddress *ThisAddress;  // 原始指针，需手动管理
    
public:
    virtual ~Socket();
};

// TCPAccept.h
class TCPAccept {
protected:
    SocketInputStream *sis = nullptr;   // 原始指针
    SocketOutputStream *sos = nullptr;  // 原始指针
};
```

**问题分析**：
- 使用原始指针容易造成内存泄漏
- 所有权不明确（谁负责释放？）
- 异常安全性差（构造函数中抛异常会泄漏）
- 难以实现移动语义

**改进建议**：
```cpp
#include <memory>

class Socket {
protected:
    std::unique_ptr<IPAddress> ThisAddress;  // 独占所有权
    
public:
    // 自动管理，无需显式析构
    virtual ~Socket() = default;
    
    // 支持移动语义
    Socket(Socket&&) = default;
    Socket& operator=(Socket&&) = default;
    
    // 禁止拷贝（如果不需要）
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
};

class TCPAccept {
protected:
    std::unique_ptr<SocketInputStream> sis;
    std::unique_ptr<SocketOutputStream> sos;
    
public:
    // 延迟初始化
    void ensureInputStream() {
        if (!sis) {
            sis = std::make_unique<SocketInputStream>(ThisSocket);
        }
    }
};
```

**优势**：
- ✅ 自动内存管理，无泄漏
- ✅ 异常安全
- ✅ 明确所有权语义
- ✅ 支持现代C++移动语义

---

### 1.2 错误处理机制

#### 🔴 问题：错误码与异常混用，不一致

**当前实现**：
```cpp
// 返回 bool 表示成功/失败
bool Connect(int sock, IPAddress *addr);

// 返回 int，负数表示错误
virtual int OnSocketRecv(int) = 0;

// 使用宏返回，不够现代
#define RETURN_FALSE return(false)
#define RETURN_ERROR(x) return(x)
```

**问题分析**：
- 错误信息丢失（仅知道失败，不知原因）
- 需要额外调用 `GetLastSocketError()` 获取详情
- 混用多种错误报告方式
- 难以传递复杂错误信息

**改进建议**：

**方案1：使用 std::expected (C++23) 或 tl::expected**
```cpp
#include <expected>  // C++23
// 或者使用 https://github.com/TartanLlama/expected

// 定义错误类型
enum class SocketErrorCode {
    Success,
    ConnectionRefused,
    Timeout,
    InvalidAddress,
    // ...
};

struct SocketError {
    SocketErrorCode code;
    std::string message;
    int native_errno;
};

// 使用 expected
std::expected<void, SocketError> Connect(int sock, IPAddress *addr) {
    if (connect(sock, ...) < 0) {
        return std::unexpected(SocketError{
            .code = SocketErrorCode::ConnectionRefused,
            .message = "Failed to connect to server",
            .native_errno = GetLastSocketError()
        });
    }
    return {};
}

// 调用示例
auto result = Connect(sock, addr);
if (!result) {
    const auto& error = result.error();
    LOG_ERROR("Connection failed: " << error.message 
              << " (errno: " << error.native_errno << ")");
    return;
}
```

**方案2：异常 + RAII**
```cpp
class SocketException : public std::runtime_error {
    SocketErrorCode code_;
    int native_errno_;
public:
    SocketException(SocketErrorCode code, const std::string& msg, int err)
        : std::runtime_error(msg), code_(code), native_errno_(err) {}
    
    SocketErrorCode code() const { return code_; }
    int native_errno() const { return native_errno_; }
};

void Connect(int sock, IPAddress *addr) {
    if (connect(sock, ...) < 0) {
        throw SocketException(
            SocketErrorCode::ConnectionRefused,
            "Failed to connect to server",
            GetLastSocketError()
        );
    }
}

// 使用
try {
    Connect(sock, addr);
} catch (const SocketException& e) {
    LOG_ERROR("Connection failed: " << e.what());
}
```

**优势**：
- ✅ 错误信息完整
- ✅ 类型安全
- ✅ 强制错误处理（expected）
- ✅ 支持错误链传递

---

### 1.3 线程安全问题

#### 🔴 问题：SocketManage 非线程安全，文档注明但容易误用

**当前实现**：
```cpp
// SocketManage.h 注释
/**
 * 该类所有函数均为非线程安全，所以不可以直接在多线程中使用
 */
class SocketManage {
    Map<int, TCPAccept *> socket_list;
    // ...
};
```

**问题分析**：
- 依赖文档说明，编译器无法检查
- 容易被多线程误用
- 缺乏线程安全的替代方案

**改进建议**：

**方案1：添加线程检查**
```cpp
class SocketManage {
private:
    std::thread::id owner_thread_id_;
    
    void CheckThread() const {
        if (std::this_thread::get_id() != owner_thread_id_) {
            throw std::runtime_error(
                "SocketManage accessed from wrong thread!"
            );
        }
    }
    
public:
    SocketManage(int max_user) 
        : owner_thread_id_(std::this_thread::get_id()) {
        // ...
    }
    
    bool Join(TCPAccept *s) {
        CheckThread();  // 运行时检查
        // ...
    }
};
```

**方案2：提供线程安全版本**
```cpp
class ThreadSafeSocketManage : public SocketManage {
private:
    mutable std::mutex mutex_;
    
public:
    bool Join(TCPAccept *s) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return SocketManage::Join(s);
    }
    
    int Update(const double &time_out) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return SocketManage::Update(time_out);
    }
};
```

**方案3：使用无锁数据结构**
```cpp
#include <atomic>
#include <concurrent_queue.h>  // Intel TBB or similar

class LockFreeSocketManage {
private:
    std::atomic<int> socket_count_{0};
    concurrent_queue<TCPAccept*> join_queue_;
    concurrent_queue<TCPAccept*> unjoin_queue_;
    
public:
    bool Join(TCPAccept *s) {
        join_queue_.push(s);
        return true;
    }
    
    int Update(const double &time_out) {
        // 处理队列中的Join/Unjoin请求
        TCPAccept* s;
        while (join_queue_.try_pop(s)) {
            // 添加到内部列表
        }
        // ...
    }
};
```

---

### 1.4 资源泄漏风险

#### 🔴 问题：手动管理socket文件描述符容易泄漏

**当前实现**：
```cpp
class Socket {
public:
    int ThisSocket;  // 公开的文件描述符
    
    virtual void CloseSocket() {
        if (ThisSocket >= 0) {
            close(ThisSocket);
            ThisSocket = -1;
        }
    }
};
```

**问题分析**：
- 公开的 `ThisSocket` 可能被外部修改
- 忘记调用 `CloseSocket()` 会泄漏
- 拷贝对象会导致重复关闭

**改进建议**：
```cpp
class SocketHandle {
private:
    int fd_;
    
public:
    explicit SocketHandle(int fd = -1) : fd_(fd) {}
    
    ~SocketHandle() {
        close();
    }
    
    // 移动语义
    SocketHandle(SocketHandle&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }
    
    SocketHandle& operator=(SocketHandle&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }
    
    // 禁止拷贝
    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;
    
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
    int get() const { return fd_; }
    int release() { int temp = fd_; fd_ = -1; return temp; }
    explicit operator bool() const { return fd_ >= 0; }
};

class Socket {
private:
    SocketHandle socket_handle_;  // RAII管理
    
public:
    int GetSocket() const { return socket_handle_.get(); }
};
```

---

### 1.5 虚函数性能开销

#### 🟡 问题：过度使用虚函数可能影响性能

**当前实现**：
```cpp
class IOSocket : public Socket {
    virtual int OnRecv(int recv_buf_size = -1, const double ct = 0);
    virtual int OnSend(int send_buf_size);
    virtual void OnError(int errno_number);
    virtual void OnClose();
    virtual bool OnUpdate() = 0;  // 纯虚函数
};
```

**问题分析**：
- 每个虚函数调用都有vtable查找开销
- 在高频调用场景（每帧数千次）会累积开销
- 阻止编译器内联优化

**改进建议**：

**方案1：CRTP (Curiously Recurring Template Pattern)**
```cpp
template<typename Derived>
class IOSocket : public Socket {
public:
    int OnRecv(int recv_buf_size = -1, const double ct = 0) {
        return static_cast<Derived*>(this)->OnRecvImpl(recv_buf_size, ct);
    }
    
    int OnSend(int send_buf_size) {
        return static_cast<Derived*>(this)->OnSendImpl(send_buf_size);
    }
    
    bool OnUpdate() {
        return static_cast<Derived*>(this)->OnUpdateImpl();
    }
};

class MyTCPAccept : public IOSocket<MyTCPAccept> {
public:
    int OnRecvImpl(int recv_buf_size, const double ct) {
        // 实现，无虚函数开销
    }
    
    bool OnUpdateImpl() {
        // 实现
    }
};
```

**方案2：使用 std::function (灵活性更高)**
```cpp
class IOSocket : public Socket {
private:
    std::function<int(int, double)> recv_handler_;
    std::function<int(int)> send_handler_;
    std::function<bool()> update_handler_;
    
public:
    void SetRecvHandler(std::function<int(int, double)> handler) {
        recv_handler_ = std::move(handler);
    }
    
    int OnRecv(int recv_buf_size, const double ct) {
        return recv_handler_ ? recv_handler_(recv_buf_size, ct) : 0;
    }
};
```

**性能对比**：
- 虚函数：~3-5ns 每次调用（vtable查找）
- CRTP：0ns（编译时解析，可内联）
- std::function：~10-15ns（间接调用）

**建议**：
- 热路径（OnRecv/OnSend）使用 CRTP
- 冷路径（OnError/OnClose）保持虚函数
- 平衡性能与代码可维护性

---

## 2. 命名不规范问题

### 2.1 不一致的命名风格

#### 🔴 问题：混用多种命名约定

**问题示例**：
```cpp
// PascalCase（类名）
class TCPSocket;
class SocketManage;

// camelCase（成员变量）
int ThisSocket;          // 大写开头
IPAddress *ThisAddress;  // 大写开头

// snake_case（部分变量）
fd_set accept_set;
struct timeval accept_timeout;

// 匈牙利命名（部分）
SocketInputStream *sis;  // sis = socket input stream?
SocketOutputStream *sos; // sos = socket output stream?

// 全大写（常量）
constexpr uint HGL_NETWORK_MAX_PORT = 65535;

// 带前缀（宏）
#define RETURN_FALSE
#define RETURN_ERROR(x)
```

**改进建议**：

**统一命名规范**：
```cpp
// 1. 类名：PascalCase
class TcpSocket;
class SocketManager;  // Manage -> Manager（更标准）

// 2. 成员变量：小写+下划线，带后缀
class Socket {
private:
    int socket_fd_;              // fd = file descriptor
    std::unique_ptr<IPAddress> address_;
    
protected:
    // 受保护成员可选不加下划线
    int socket_fd;
    
public:
    // 访问器
    int socket_fd() const { return socket_fd_; }
};

// 3. 函数名：PascalCase（公有）或camelCase（私有）
class TcpSocket {
public:
    bool Connect();           // 公有接口：PascalCase
    void SetNodelay(bool);
    
private:
    void resetConnection();   // 私有方法：camelCase
    bool initSocket();
};

// 4. 常量：kPascalCase 或 UPPER_SNAKE_CASE
namespace network {
    constexpr uint32_t kMaxPort = 65535;
    constexpr uint32_t kDefaultTimeout = 60;
    // 或者
    constexpr uint32_t MAX_PORT = 65535;
}

// 5. 缩写：避免过度缩写
SocketInputStream *input_stream_;   // 而不是 sis
SocketOutputStream *output_stream_; // 而不是 sos

// 6. 避免匈牙利命名
int max_connections;  // 而不是 nMaxConnections
bool is_connected;    // 而不是 bIsConnected
```

---

### 2.2 容易混淆的命名

#### 🔴 问题：命名不够清晰

**问题示例**：
```cpp
// TCPAccept - Accept 是动词还是名词？
class TCPAccept;  // 实际是名词：被接受的连接

// SocketManage - 缺少动词或明确角色
class SocketManage;  // Manager? Management?

// IOSocket - IO太通用
class IOSocket;

// ProcRecv - Proc是什么？Process?
void ProcSocketRecvList();
```

**改进建议**：
```cpp
// 更清晰的命名
class TcpConnection;         // 而不是 TCPAccept
class TcpAcceptedConnection; // 或者更明确

class SocketManager;         // 而不是 SocketManage

class StreamSocket;          // 而不是 IOSocket
class BiDirectionalSocket;   // 更明确

void ProcessRecvList();      // 而不是 ProcSocketRecvList
void HandleReceivedSockets();
```

---

### 2.3 函数命名问题

#### 🔴 问题：动词使用不一致

**问题示例**：
```cpp
class SocketManage {
    bool Join(TCPAccept *s);      // 加入
    bool Unjoin(TCPAccept *s);    // Un-join？
    
    void ProcSocketRecvList();    // Proc = Process
    int Update(const double &);   // Update做了很多事
};

class Socket {
    void CloseSocket();           // Close + Socket重复
    bool ReCreateSocket();        // Re-Create，驼峰不一致
};
```

**改进建议**：
```cpp
class SocketManager {
    // 对称命名
    bool Add(TcpConnection* conn);     // 而不是 Join
    bool Remove(TcpConnection* conn);  // 而不是 Unjoin
    
    // 或者
    bool Register(TcpConnection* conn);
    bool Unregister(TcpConnection* conn);
    
    // 明确动词
    void ProcessRecvList();    // 而不是 ProcSocketRecvList
    void ProcessSendList();
    
    // 分解Update
    int Poll(double timeout);          // 轮询事件
    void DispatchEvents();             // 分发事件
    void CleanupErrors();              // 清理错误
};

class Socket {
    void Close();           // 而不是 CloseSocket（上下文已知）
    bool Recreate();        // 而不是 ReCreateSocket
};
```

---

## 3. 架构改进建议

### 3.1 依赖注入与解耦

#### 🟡 问题：硬编码依赖

**当前实现**：
```cpp
class SocketManage {
private:
    SocketManageBase *manage;  // 工厂创建，但类型固定
    
public:
    SocketManage(int max_user) {
        manage = CreateSocketManageBase(max_user);  // 平台特定
    }
};
```

**改进建议**：
```cpp
// 1. 接口分离
class IEventLoop {
public:
    virtual ~IEventLoop() = default;
    virtual bool Add(int fd) = 0;
    virtual bool Remove(int fd) = 0;
    virtual int Poll(double timeout, EventList& events) = 0;
};

class EpollEventLoop : public IEventLoop { /*...*/ };
class KqueueEventLoop : public IEventLoop { /*...*/ };
class SelectEventLoop : public IEventLoop { /*...*/ };

// 2. 依赖注入
class SocketManager {
private:
    std::unique_ptr<IEventLoop> event_loop_;
    
public:
    explicit SocketManager(std::unique_ptr<IEventLoop> event_loop)
        : event_loop_(std::move(event_loop)) {}
    
    // 工厂方法
    static std::unique_ptr<SocketManager> CreateDefault(int max_connections) {
        auto event_loop = CreatePlatformEventLoop(max_connections);
        return std::make_unique<SocketManager>(std::move(event_loop));
    }
};

// 3. 使用
auto manager = SocketManager::CreateDefault(1000);
// 或者测试时注入mock
auto manager = SocketManager(std::make_unique<MockEventLoop>());
```

---

### 3.2 异步I/O支持

#### 🟡 问题：仅支持同步和多线程模式

**改进建议**：添加协程支持

```cpp
#include <coroutine>

// 异步连接
class AsyncTcpClient {
public:
    struct ConnectAwaiter {
        TcpClient* client_;
        
        bool await_ready() { return false; }
        
        void await_suspend(std::coroutine_handle<> handle) {
            client_->SetConnectCallback([handle]() mutable {
                handle.resume();
            });
        }
        
        bool await_resume() { 
            return client_->IsConnected();
        }
    };
    
    ConnectAwaiter AsyncConnect(const IPAddress* addr) {
        // 启动非阻塞连接
        return ConnectAwaiter{this};
    }
};

// 使用示例
Task<void> ConnectAndSend() {
    AsyncTcpClient client;
    
    // 异步连接，不阻塞线程
    bool connected = co_await client.AsyncConnect(addr);
    if (!connected) {
        co_return;
    }
    
    // 异步发送
    co_await client.AsyncSend(data, size);
    
    // 异步接收
    auto response = co_await client.AsyncRecv();
}
```

---

### 3.3 对象池优化

#### 🟡 问题：频繁new/delete导致性能问题

**当前实现**：
```cpp
// 每个连接都new
TCPAccept* accept = new TCPAccept();
// ...
delete accept;  // 断开时delete
```

**改进建议**：
```cpp
template<typename T>
class ObjectPool {
private:
    std::vector<std::unique_ptr<T>> pool_;
    std::vector<T*> available_;
    std::mutex mutex_;
    
public:
    ObjectPool(size_t initial_size = 100) {
        pool_.reserve(initial_size);
        available_.reserve(initial_size);
        
        for (size_t i = 0; i < initial_size; ++i) {
            auto obj = std::make_unique<T>();
            available_.push_back(obj.get());
            pool_.push_back(std::move(obj));
        }
    }
    
    T* Acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (available_.empty()) {
            // 动态扩展
            auto obj = std::make_unique<T>();
            T* ptr = obj.get();
            pool_.push_back(std::move(obj));
            return ptr;
        }
        
        T* obj = available_.back();
        available_.pop_back();
        obj->Reset();  // 重置状态
        return obj;
    }
    
    void Release(T* obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        available_.push_back(obj);
    }
};

// 使用
class TcpServer {
private:
    ObjectPool<TcpConnection> connection_pool_;
    
public:
    void OnNewConnection(int fd, IPAddress* addr) {
        auto* conn = connection_pool_.Acquire();
        conn->Initialize(fd, addr);
        // ...
    }
    
    void OnConnectionClosed(TcpConnection* conn) {
        connection_pool_.Release(conn);
    }
};
```

---

## 4. 扩展特性建议

### 4.1 SSL/TLS 支持

```cpp
class SslContext {
private:
    SSL_CTX* ctx_;
    
public:
    SslContext(const std::string& cert_file, 
               const std::string& key_file);
    ~SslContext();
    
    SSL* CreateSslHandle();
};

class SslTcpSocket : public TcpSocket {
private:
    std::unique_ptr<SslContext> ssl_context_;
    SSL* ssl_handle_ = nullptr;
    
public:
    bool SslConnect() {
        ssl_handle_ = ssl_context_->CreateSslHandle();
        SSL_set_fd(ssl_handle_, socket_fd_);
        return SSL_connect(ssl_handle_) == 1;
    }
    
    int Send(const void* data, size_t size) override {
        return SSL_write(ssl_handle_, data, size);
    }
    
    int Recv(void* buffer, size_t size) override {
        return SSL_read(ssl_handle_, buffer, size);
    }
};
```

---

### 4.2 HTTP/2 支持

```cpp
class Http2Connection {
private:
    nghttp2_session* session_;
    
public:
    void SendRequest(const Http2Request& request) {
        // 使用nghttp2库实现
    }
    
    void OnFrameReceived(const nghttp2_frame* frame) {
        // 处理HTTP/2帧
    }
};
```

---

### 4.3 连接池管理

```cpp
class ConnectionPool {
private:
    std::string host_;
    uint16_t port_;
    size_t max_connections_;
    
    std::queue<std::unique_ptr<TcpClient>> idle_connections_;
    std::unordered_set<TcpClient*> active_connections_;
    
public:
    std::unique_ptr<TcpClient> Acquire() {
        if (!idle_connections_.empty()) {
            auto conn = std::move(idle_connections_.front());
            idle_connections_.pop();
            
            if (conn->IsConnected()) {
                active_connections_.insert(conn.get());
                return conn;
            }
        }
        
        // 创建新连接
        auto conn = std::make_unique<TcpClient>();
        conn->Connect(host_, port_);
        active_connections_.insert(conn.get());
        return conn;
    }
    
    void Release(std::unique_ptr<TcpClient> conn) {
        active_connections_.erase(conn.get());
        
        if (conn->IsConnected() && 
            idle_connections_.size() < max_connections_) {
            idle_connections_.push(std::move(conn));
        }
    }
};
```

---

### 4.4 性能监控

```cpp
class PerformanceMetrics {
private:
    std::atomic<uint64_t> total_bytes_sent_{0};
    std::atomic<uint64_t> total_bytes_received_{0};
    std::atomic<uint64_t> total_connections_{0};
    std::atomic<uint64_t> active_connections_{0};
    
    std::chrono::steady_clock::time_point start_time_;
    
public:
    void RecordBytesSent(uint64_t bytes) {
        total_bytes_sent_.fetch_add(bytes);
    }
    
    void RecordBytesReceived(uint64_t bytes) {
        total_bytes_received_.fetch_add(bytes);
    }
    
    double GetThroughputMbps() const {
        auto duration = std::chrono::steady_clock::now() - start_time_;
        auto seconds = std::chrono::duration<double>(duration).count();
        auto total_bytes = total_bytes_sent_ + total_bytes_received_;
        return (total_bytes * 8.0) / (seconds * 1000000.0);
    }
    
    nlohmann::json ToJson() const {
        return {
            {"total_bytes_sent", total_bytes_sent_.load()},
            {"total_bytes_received", total_bytes_received_.load()},
            {"active_connections", active_connections_.load()},
            {"throughput_mbps", GetThroughputMbps()}
        };
    }
};
```

---

### 4.5 配置管理

```cpp
struct NetworkConfig {
    uint32_t tcp_buffer_size = 256 * 1024;
    double timeout_seconds = 60.0;
    double heartbeat_seconds = 30.0;
    bool tcp_nodelay = true;
    uint32_t max_connections = 1000;
    
    // 从配置文件加载
    static NetworkConfig FromFile(const std::string& path) {
        // 使用JSON/YAML/TOML解析
    }
};

class TcpServer {
private:
    NetworkConfig config_;
    
public:
    explicit TcpServer(NetworkConfig config = {})
        : config_(std::move(config)) {}
};
```

---

## 5. 测试改进建议

### 5.1 单元测试

```cpp
#include <gtest/gtest.h>

TEST(SocketTest, CreateAndClose) {
    auto addr = std::make_unique<IPv4Address>("127.0.0.1", 8080, 
                                               SOCK_STREAM, IPPROTO_TCP);
    Socket socket;
    
    ASSERT_TRUE(socket.InitSocket(addr.get()));
    EXPECT_GE(socket.GetSocket(), 0);
    
    socket.Close();
    EXPECT_LT(socket.GetSocket(), 0);
}

TEST(TcpClientTest, ConnectTimeout) {
    auto addr = std::make_unique<IPv4Address>("192.0.2.1", 12345,  // 不可达地址
                                               SOCK_STREAM, IPPROTO_TCP);
    TcpClient client;
    client.SetTimeout(1.0);  // 1秒超时
    
    auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(client.Connect(addr.get()));
    auto duration = std::chrono::steady_clock::now() - start;
    
    EXPECT_LE(duration, std::chrono::seconds(2));  // 应该在2秒内超时
}
```

---

### 5.2 集成测试

```cpp
TEST(TcpServerTest, AcceptAndEcho) {
    // 启动服务器
    TcpServer server;
    auto server_addr = std::make_unique<IPv4Address>(8080);
    server.CreateServerSocket(server_addr.get());
    
    // 在独立线程中运行
    std::thread server_thread([&]() {
        SocketManager manager(10);
        while (/* running */) {
            manager.Poll(1.0);
        }
    });
    
    // 客户端连接
    TcpClient client;
    auto client_addr = std::make_unique<IPv4Address>("127.0.0.1", 8080);
    ASSERT_TRUE(client.Connect(client_addr.get()));
    
    // 发送数据
    std::string message = "Hello, Server!";
    client.Send(message.data(), message.size());
    
    // 接收回显
    char buffer[1024];
    int received = client.Recv(buffer, sizeof(buffer));
    EXPECT_EQ(received, message.size());
    EXPECT_EQ(std::string(buffer, received), message);
    
    server_thread.join();
}
```

---

## 6. 文档改进建议

### 6.1 API 文档

使用 Doxygen 风格注释：

```cpp
/**
 * @brief TCP客户端类，支持阻塞和非阻塞模式
 * 
 * @details 
 * TCPClient提供了简单易用的TCP客户端接口，支持：
 * - 阻塞和非阻塞连接
 * - 超时控制
 * - 心跳机制
 * - 自动重连（可选）
 * 
 * @example
 * @code
 * auto addr = CreateIPv4TCP("example.com", 80);
 * TcpClient client;
 * if (client.Connect(addr)) {
 *     client.Send("GET / HTTP/1.1\r\n\r\n");
 *     char buffer[4096];
 *     int received = client.Recv(buffer, sizeof(buffer));
 * }
 * @endcode
 * 
 * @note 线程安全：此类不是线程安全的，不应在多线程中共享
 * @warning 必须在使用前调用Connect()
 */
class TcpClient : public TcpSocket {
public:
    /**
     * @brief 连接到服务器
     * 
     * @param addr 服务器地址，不能为空
     * @return true 连接成功
     * @return false 连接失败，调用GetLastError()获取错误信息
     * 
     * @throws SocketException 如果地址无效
     * 
     * @see Disconnect(), IsConnected()
     */
    bool Connect(const IPAddress* addr);
};
```

---

### 6.2 架构文档

添加详细的架构图和说明文档。

---

## 7. 总结与优先级

### 高优先级改进（立即实施）

1. ✅ **使用智能指针** - 解决内存泄漏问题
2. ✅ **统一命名规范** - 提高代码可读性
3. ✅ **添加错误处理机制** - std::expected 或异常
4. ✅ **RAII管理资源** - 自动管理socket文件描述符

### 中优先级改进（短期规划）

1. 🔄 **线程安全检查** - 防止误用
2. 🔄 **对象池** - 提升性能
3. 🔄 **单元测试** - 保证质量
4. 🔄 **文档完善** - Doxygen注释

### 低优先级改进（长期规划）

1. 📋 **协程支持** - 异步I/O
2. 📋 **SSL/TLS** - 安全通信
3. 📋 **HTTP/2** - 现代协议
4. 📋 **性能监控** - 运维支持

---

## 8. 实施建议

### 渐进式重构

不要一次性重写所有代码，而是：

1. **新代码使用新标准** - 新功能采用改进后的设计
2. **逐步重构旧代码** - 按模块逐步迁移
3. **保持向后兼容** - 提供过渡期的兼容层
4. **充分测试** - 每次改动都要测试

### 示例：重构计划

```cpp
// Phase 1: 添加新接口（保持旧接口）
class Socket {
public:
    // 新接口（推荐）
    int socket_fd() const { return socket_handle_.get(); }
    
    // 旧接口（标记为deprecated）
    [[deprecated("Use socket_fd() instead")]]
    int ThisSocket() const { return socket_handle_.get(); }
};

// Phase 2: 更新文档，标记旧接口废弃

// Phase 3: 提供迁移工具

// Phase 4: 移除旧接口（下个大版本）
```

---

这份文档提供了全面的改进建议，涵盖了设计、命名、架构、扩展特性等多个方面。建议根据项目实际情况和资源，分阶段实施这些改进。
