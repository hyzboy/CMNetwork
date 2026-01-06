#pragma once

#include<hgl/network/IP.h>

#if HGL_OS == HGL_OS_Windows
    #include<winsock2.h>
#else
    #include<unistd.h>
#endif

namespace hgl
{
    namespace network
    {
        /**
         * RAII wrapper for socket file descriptor
         * Automatically closes socket on destruction
         */
        class SocketHandle
        {
        private:
            int fd_;

            void close_fd() noexcept
            {
                if (fd_ >= 0)
                {
#if HGL_OS == HGL_OS_Windows
                    closesocket(fd_);
#else
                    ::close(fd_);
#endif
                    fd_ = -1;
                }
            }

        public:
            // Default constructor - invalid socket
            SocketHandle() noexcept : fd_(-1) {}

            // Construct from file descriptor
            explicit SocketHandle(int fd) noexcept : fd_(fd) {}

            // Destructor - closes socket automatically
            ~SocketHandle() noexcept
            {
                close_fd();
            }

            // Move constructor
            SocketHandle(SocketHandle&& other) noexcept : fd_(other.fd_)
            {
                other.fd_ = -1;
            }

            // Move assignment
            SocketHandle& operator=(SocketHandle&& other) noexcept
            {
                if (this != &other)
                {
                    close_fd();
                    fd_ = other.fd_;
                    other.fd_ = -1;
                }
                return *this;
            }

            // Disable copy
            SocketHandle(const SocketHandle&) = delete;
            SocketHandle& operator=(const SocketHandle&) = delete;

            // Get the file descriptor
            int get() const noexcept { return fd_; }

            // Release ownership of the file descriptor
            int release() noexcept
            {
                int temp = fd_;
                fd_ = -1;
                return temp;
            }

            // Reset with a new file descriptor
            void reset(int fd = -1) noexcept
            {
                if (fd_ != fd)
                {
                    close_fd();
                    fd_ = fd;
                }
            }

            // Check if socket is valid
            bool is_valid() const noexcept { return fd_ >= 0; }
            explicit operator bool() const noexcept { return is_valid(); }

            // Manual close
            void close() noexcept
            {
                close_fd();
            }
        };
    }//namespace network
}//namespace hgl
