# 设计改进建议 - 快速参考

> 本文档是 DESIGN_REVIEW_AND_IMPROVEMENTS.md 的精简版，列出最重要的改进建议

## 🔴 高优先级（应立即实施）

### 1. 使用智能指针管理内存

**问题**：原始指针容易泄漏
```cpp
// ❌ 当前
class Socket {
    IPAddress *ThisAddress;  // 需要手动delete
};
```

**改进**：
```cpp
// ✅ 推荐
class Socket {
    std::unique_ptr<IPAddress> address_;  // 自动管理
};
```

---

### 2. 统一命名规范

**问题**：混用多种风格
```cpp
// ❌ 当前
int ThisSocket;              // PascalCase开头
SocketInputStream *sis;      // 过度缩写
void ProcSocketRecvList();   // Proc不明确
```

**改进**：
```cpp
// ✅ 推荐
int socket_fd_;                    // 成员变量：小写+下划线
std::unique_ptr<SocketInputStream> input_stream_;  // 清晰命名
void ProcessRecvList();            // 完整动词
```

---

### 3. 改进错误处理

**问题**：错误信息丢失
```cpp
// ❌ 当前
bool Connect(int sock, IPAddress *addr);  // 仅返回true/false
```

**改进**：
```cpp
// ✅ 方案1：使用 std::expected (C++23)
std::expected<void, SocketError> Connect(int sock, IPAddress *addr);

// ✅ 方案2：异常
void Connect(int sock, IPAddress *addr);  // 失败时抛异常
```

---

### 4. RAII管理Socket

**问题**：手动管理fd容易泄漏
```cpp
// ❌ 当前
class Socket {
public:
    int ThisSocket;  // 公开，可能被修改
    void CloseSocket();  // 忘记调用会泄漏
};
```

**改进**：
```cpp
// ✅ 推荐
class SocketHandle {
    int fd_;
public:
    explicit SocketHandle(int fd = -1) : fd_(fd) {}
    ~SocketHandle() { if (fd_ >= 0) close(fd_); }
    
    // 移动语义
    SocketHandle(SocketHandle&& other) noexcept;
    
    // 禁止拷贝
    SocketHandle(const SocketHandle&) = delete;
};
```

---

## 🟡 中优先级（短期规划）

### 5. 线程安全检查

**问题**：SocketManage非线程安全但仅靠文档说明

**改进**：
```cpp
class SocketManage {
    std::thread::id owner_thread_id_;
    
    void CheckThread() const {
        if (std::this_thread::get_id() != owner_thread_id_) {
            throw std::runtime_error("Called from wrong thread!");
        }
    }
};
```

---

### 6. 对象池避免频繁分配

**问题**：每个连接都new/delete

**改进**：
```cpp
template<typename T>
class ObjectPool {
    std::vector<std::unique_ptr<T>> pool_;
    std::vector<T*> available_;
public:
    T* Acquire();  // 复用对象
    void Release(T* obj);
};
```

---

### 7. 添加单元测试

**改进**：
```cpp
#include <gtest/gtest.h>

TEST(SocketTest, CreateAndClose) {
    Socket socket;
    ASSERT_TRUE(socket.Init());
    socket.Close();
    EXPECT_FALSE(socket.IsValid());
}
```

---

## 📋 低优先级（长期规划）

### 8. 协程异步I/O (C++20)

```cpp
Task<void> AsyncConnect() {
    bool connected = co_await client.Connect(addr);
    if (!connected) co_return;
    
    co_await client.Send(data);
    auto response = co_await client.Recv();
}
```

---

### 9. SSL/TLS支持

```cpp
class SslTcpSocket : public TcpSocket {
    SSL* ssl_handle_;
public:
    bool SslConnect();
    int SecureSend(const void* data, size_t size);
};
```

---

### 10. HTTP/2支持

```cpp
class Http2Connection {
    nghttp2_session* session_;
public:
    void SendRequest(const Http2Request& request);
};
```

---

## 命名规范速查表

| 类型 | 推荐风格 | 示例 | ❌ 避免 |
|------|---------|------|---------|
| 类名 | PascalCase | `TcpSocket` | `TCPSocket`, `tcp_socket` |
| 函数名 | PascalCase | `Connect()` | `connect()`, `doConnect()` |
| 成员变量 | 小写+下划线+后缀 | `socket_fd_` | `ThisSocket`, `m_socket` |
| 私有方法 | camelCase | `resetConnection()` | `ResetConnection()` |
| 常量 | kPascalCase | `kMaxPort` | `MAX_PORT`, `maxPort` |

---

## 常见错误模式对照

### 错误模式1：忘记delete

```cpp
// ❌ 错误
void foo() {
    IPAddress* addr = new IPv4Address(...);
    if (error) return;  // 泄漏！
    delete addr;
}

// ✅ 正确
void foo() {
    auto addr = std::make_unique<IPv4Address>(...);
    if (error) return;  // 自动清理
}
```

---

### 错误模式2：忘记关闭socket

```cpp
// ❌ 错误
int CreateSocket() {
    int fd = socket(...);
    if (error) return -1;  // fd泄漏！
    return fd;
}

// ✅ 正确
SocketHandle CreateSocket() {
    SocketHandle handle(socket(...));
    if (!handle) return {};  // 自动清理
    return handle;  // 移动语义
}
```

---

### 错误模式3：错误处理不完整

```cpp
// ❌ 错误
bool Connect() {
    if (connect(...) < 0)
        return false;  // 为什么失败？不知道
}

// ✅ 正确
std::expected<void, SocketError> Connect() {
    if (connect(...) < 0) {
        return std::unexpected(SocketError{
            .code = SocketErrorCode::ConnectionRefused,
            .message = "Connection refused",
            .native_errno = errno
        });
    }
    return {};
}
```

---

## 实施步骤

1. **第一周**：统一命名规范（新代码先用）
2. **第二周**：引入智能指针（逐个类迁移）
3. **第三周**：添加RAII封装（SocketHandle）
4. **第四周**：改进错误处理（选择方案）
5. **持续**：添加单元测试，逐步重构

---

## 参考资源

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Modern C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices)
- 详细文档：[DESIGN_REVIEW_AND_IMPROVEMENTS.md](DESIGN_REVIEW_AND_IMPROVEMENTS.md)
