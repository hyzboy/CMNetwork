#pragma once

#include<hgl/network/Socket.h>

namespace hgl
{
    namespace network
    {
        /**
         * CRTP版本的IOSocket - 消除虚函数开销
         * 使用方法：class MySocket : public IOSocketCRTP<MySocket> { ... };
         * 
         * 性能优势：
         * - OnRecv/OnSend 调用无虚函数开销（可内联）
         * - 编译时多态，更好的优化机会
         * - 适用于高频调用的热路径
         */
        template<typename Derived>
        class IOSocketCRTP : public Socket
        {
        protected:

            int64 send_total_;                                                                       ///<发送字节数统计
            int64 recv_total_;                                                                       ///<接收字节数统计

            double recv_time_out_;                                                                   ///<数据接收超时
            double last_recv_time_;                                                                  ///<最后一次接收数据的时间

        public:

            IOSocketCRTP()
            {
                Clear();
            }

            virtual ~IOSocketCRTP() = default;

            void Clear()
            {
                send_total_ = 0;
                recv_total_ = 0;
                recv_time_out_ = HGL_NETWORK_DOUBLE_TIME_OUT;
                last_recv_time_ = 0;
            }

            void CloseSocket() override                                                   ///<关闭连接
            {
                Socket::CloseSocket();
                Clear();
            }

            const double& GetRecvTimeOut() const { return recv_time_out_; }                    ///<取得接收数据超时时间
            void SetRecvTimeOut(const double to) { recv_time_out_ = to; }              ///<设置接收数据超时时间

        public: //事件函数 - 使用CRTP静态分发，无虚函数开销

            /**
             * 接收数据处理事件函数（CRTP版本 - 无虚函数开销）
             * @param recv_buf_size 缓冲区内的数据长度
             * @param ct 当前时间
             * @return 成功获取的数据字节数
             */
            int OnRecv(int recv_buf_size = -1, const double ct = 0)
            {
                last_recv_time_ = ct;
                return static_cast<Derived*>(this)->OnRecvImpl(recv_buf_size, ct);
            }

            /**
             * 发送数据处理事件函数（CRTP版本 - 无虚函数开销）
             * @param send_buf_size 发送数据缓冲区内可用最大长度
             * @return 成功发送的数据字节数
             */
            int OnSend(int send_buf_size)
            {
                return static_cast<Derived*>(this)->OnSendImpl(send_buf_size);
            }

            /**
             * 错误处理事件函数（CRTP版本 - 无虚函数开销）
             * @param errno_number 错误号
             */
            void OnError(int errno_number)
            {
                static_cast<Derived*>(this)->OnErrorImpl(errno_number);
            }

            /**
             * 关闭事件函数（CRTP版本 - 无虚函数开销）
             */
            void OnClose()
            {
                static_cast<Derived*>(this)->OnCloseImpl();
            }

            /**
             * 刷新事件函数（CRTP版本 - 无虚函数开销）
             * @return 是否正常
             */
            bool OnUpdate()
            {
                return static_cast<Derived*>(this)->OnUpdateImpl();
            }

        public: //属性函数

            const int64 GetSendTotal() const { return send_total_; }                         ///<取得发送字节数统计
            const int64 GetRecvTotal() const { return recv_total_; }                         ///<取得接收字节数统计

            const void RestartLastRecvTime() { last_recv_time_ = 0; }                        ///<复位最后获取数据时间
            const double GetLastRecvTime() const { return last_recv_time_; }                  ///<取得最后获取数据时间
            const bool CheckRecvTimeOut(const double ct)                                      ///<检测是否超时
            {
                if ((last_recv_time_ > 0)
                    && (last_recv_time_ + recv_time_out_ < ct))
                    return(true);

                return(false);
            }

        protected:
            // Derived classes must implement these methods (CRTP requirement)
            // No default implementations provided - compiler will check at compile-time
            
            // If your compiler reports these functions are not found, 
            // please implement them in your derived class:
            // int OnRecvImpl(int recv_buf_size, const double ct);
            // int OnSendImpl(int send_buf_size);
            // void OnErrorImpl(int errno_number);
            // void OnCloseImpl();
            // bool OnUpdateImpl();
        };//class IOSocketCRTP

        /**
         * 使用示例：
         * 
         * class MyTCPConnection : public IOSocketCRTP<MyTCPConnection>
         * {
         * public:
         *     int OnRecvImpl(int recv_buf_size, const double ct) {
         *         // 实现接收逻辑，无虚函数开销
         *         return recv_bytes;
         *     }
         *     
         *     int OnSendImpl(int send_buf_size) {
         *         // 实现发送逻辑，无虚函数开销
         *         return sent_bytes;
         *     }
         *     
         *     bool OnUpdateImpl() {
         *         // 实现更新逻辑
         *         return true;
         *     }
         * };
         */
    }//namespace network
}//namespace hgl
