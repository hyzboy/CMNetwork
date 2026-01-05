# CMNetwork Architecture Overview

## Executive Summary

CMNetwork is a cross-platform C++ network communication library that provides object-oriented abstractions for TCP, UDP, SCTP, and WebSocket protocols. The library implements platform-specific optimizations using epoll (Linux), kqueue (BSD/macOS), and select (Windows) for efficient event-driven I/O.

## Core Design Principles

1. **Layered Architecture**: Clear separation between transport (TCP/UDP/SCTP) and application (HTTP/WebSocket) layers
2. **Platform Abstraction**: Unified API with platform-specific implementations
3. **Object-Oriented**: Class hierarchy for extensibility and maintainability
4. **Event-Driven**: Non-blocking I/O with efficient event notification mechanisms
5. **Stream-Based I/O**: Java-like InputStream/OutputStream interfaces

## Key Components

### 1. Socket Base Classes

```
Socket (base)
├── TCPSocket
│   ├── TCPClient
│   └── TCPAccept (server-side connections)
├── UdpSocket
└── SCTPSocket
    ├── SCTPO2OSocket (one-to-one)
    └── SCTPO2MSocket (one-to-many)
```

### 2. IP Address Management

- `IPAddress` - Abstract base for IPv4/IPv6
- `IPv4Address` - IPv4 implementation with broadcast support
- `IPv6Address` - IPv6 implementation
- Protocol binding (TCP/UDP/SCTP/UDP-Lite)
- Domain name resolution

### 3. Socket Management

- `SocketManage` - Unified socket lifecycle management
- `SocketManageBase` - Abstract platform interface
  - `SocketManageEpoll` - Linux (edge-triggered epoll)
  - `SocketManageKqueue` - BSD/macOS
  - `SocketManageSelect` - Windows fallback

### 4. Server Architecture

```
AcceptServer (base)
└── TCPServer
    ├── ServerSocket (listening)
    ├── MultiThreadAccept (multi-threaded accept)
    └── SocketManage (connection pool)
```

### 5. Protocol Support

- **TCP**: Connection-oriented, reliable, stream-based
  - TCP_NODELAY support (Nagle algorithm control)
  - Keep-alive mechanism
  - Application-level heartbeat
  
- **UDP**: Connectionless, datagram-based
  - Unicast, broadcast, multicast
  - UDP-Lite variant support
  
- **SCTP**: Message-oriented, reliable
  - Multi-streaming
  - One-to-one and one-to-many modes
  
- **WebSocket**: Real-time bidirectional communication
  - Handshake processing
  - Sec-WebSocket-Key validation

## Technical Highlights

### Event-Driven I/O

**Linux (Epoll)**
```cpp
// Edge-triggered mode for maximum performance
ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLRDHUP;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);
```

**BSD/macOS (Kqueue)**
```cpp
EV_SET(&change, sock, EVFILT_READ, EV_ADD, 0, 0, nullptr);
kevent(kqueue_fd, &change, 1, nullptr, 0, nullptr);
```

### Multi-Threading Support

- **Multi-threaded Accept**: Multiple threads accepting connections concurrently
- **Thread-per-connection**: Each connection can have dedicated send/recv threads
- **Lock-free Design**: SocketManage is single-threaded for simplicity

### Connection Management

- Automatic timeout handling (default: 60 seconds)
- Application-level heartbeat (default: 30 seconds)
- Keep-alive support with configurable parameters
- Error detection and cleanup

### Buffer Management

- 256KB TCP buffer size (configurable)
- Stream-based I/O with internal buffering
- Zero-copy sendfile support (Linux/FreeBSD)

## Performance Optimizations

1. **Edge-Triggered Mode**: Reduces system calls (Linux epoll)
2. **Non-blocking I/O**: Prevents thread blocking
3. **Object Pooling**: Reuses IPAddress objects
4. **Batch Operations**: Supports bulk join/unjoin
5. **TCP_NODELAY**: Configurable for low-latency applications

## Platform Support

| Platform | I/O Multiplexing | Status |
|----------|------------------|--------|
| Linux    | epoll (edge-triggered) | ✓ Optimal |
| FreeBSD  | kqueue | ✓ Optimal |
| OpenBSD  | kqueue | ✓ Optimal |
| NetBSD   | kqueue | ✓ Optimal |
| macOS    | kqueue | ✓ Optimal |
| Windows  | select | ✓ Compatible |

## Error Handling

### Socket Error Enumeration
```cpp
enum SocketError {
    nseNoError,         // No error
    nseWouldBlock,      // Non-blocking operation pending
    nseSoftwareBreak,   // Local disconnect
    nsePeerBreak,       // Remote disconnect
    nseTimeOut,         // Operation timeout
    nseBrokenPipe,      // Broken pipe
};
```

### Logging System
- Object-level logging macros
- Error string lookup
- Debug statistics (optional compile-time)

## Data Flow

### TCP Client
```
Application
  ↓ OutputStream
  ↓ SocketOutputStream
  ↓ send()
  ↓ Kernel TCP Buffer
  ↓ Network
```

### TCP Server
```
ServerSocket (listen)
  ↓ accept()
  ↓ AcceptThread
  ↓ TCPAccept
  ↓ SocketManage
  ↓ Event Loop
    ├─ Recv Event → ProcRecv
    ├─ Send Event → ProcSend
    └─ Error Event → ProcError
```

## Usage Patterns

### TCP Client Example
```cpp
IPv4Address *addr = CreateIPv4TCP("example.com", 80);
TCPClient *client = new TCPClient();

if (client->CreateConnect(addr)) {
    client->Heart = 30.0;   // 30s heartbeat
    client->TimeOut = 60.0; // 60s timeout
    
    auto *out = client->GetOutputStream();
    out->WriteUTF8String("Hello");
    
    auto *in = client->GetInputStream();
    U8String response;
    in->ReadUTF8String(response);
    
    client->Disconnect();
}
```

### TCP Server Example
```cpp
TCPServer server;
IPv4Address *addr = CreateIPv4TCP(8080);
server.CreateServerSocket(addr);

MultiThreadAccept<MyAcceptThread> mta;
mta.Init(&server, 4);  // 4 accept threads
mta.Start();

SocketManage socket_manage(1000);
while (running) {
    socket_manage.Update(5.0);
    
    // Handle errors
    auto &errors = socket_manage.GetErrorSocketSet();
    for (auto *conn : errors) {
        socket_manage.Unjoin(conn);
        delete conn;
    }
}
```

## Strengths

1. **Well-Architected**: Clean separation of concerns, easy to understand
2. **Cross-Platform**: Comprehensive platform support with optimal implementations
3. **High Performance**: Modern event-driven architecture, supports high concurrency
4. **Developer-Friendly**: OOP interfaces, stream I/O, extensive utilities
5. **Reliable**: Robust error handling, connection management, timeout protection
6. **Extensible**: Modular design, easy to add protocols and features

## Potential Improvements

1. **Async API**: Full async/await support (C++20 coroutines)
2. **SSL/TLS**: Integrated secure communication
3. **HTTP/2 & QUIC**: Modern HTTP protocol support
4. **Connection Pooling**: Client-side connection management
5. **Smart Pointers**: Modern C++ memory management
6. **Performance Metrics**: Built-in monitoring and statistics

## Use Cases

- High-performance server applications
- Real-time communication systems
- Game servers
- IoT device communication
- Distributed systems
- Network proxies and gateways

## Conclusion

CMNetwork is a mature, well-designed network library suitable for production use in C++ applications requiring reliable, high-performance network communication. Its clean architecture, comprehensive platform support, and extensive feature set make it an excellent foundation for building networked applications.
