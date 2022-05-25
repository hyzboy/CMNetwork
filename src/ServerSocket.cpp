﻿#include<hgl/network/ServerSocket.h>
#include<hgl/LogInfo.h>

namespace hgl
{
    namespace network
    {
        ServerSocket::ServerSocket()
        {
            ThisSocket=-1;
            server_address=nullptr;
        }

        ServerSocket::~ServerSocket()
        {
            CloseServer();
        }

        /**
         * 创建多个空的IP地址空间
         * @param ip_buffer 存放IP地址空间的缓冲区
         * @param count 需要产生的IP地址空间数量
         * @return 是否创建成功
         */
        bool ServerSocket::CreateIPAddress(IPAddress **ip_buffer,int count)const
        {
            if(!server_address)return(false);
            if(!ip_buffer)return(false);
            if(count<=0)return(false);

            for(int i=0;i<count;i++)
            {
                *ip_buffer=server_address->Create();
                ip_buffer++;
            }

            return(true);
        }

        /**
        * 创建服务器
        * @param addr 服务器地址
        * @param max_listen 最大监听数量(指同一时间在未处理的情况下，最多有多少个连接可以被处理。注：并非越大越好)
        * @param reuse 是否可以复用这个IP地址，默认为假
        * @return 创建服务器是否成功
        */
        bool ServerSocket::CreateServer(const IPAddress *addr,const uint max_listen,bool reuse)
        {
            ThisSocket=CreateServerSocket();

            if(ThisSocket<0)
            {
                LOG_HINT(OS_TEXT("Create TCP Server Socket Failed!"));
                return(false);
            }

            if(!addr->Bind(ThisSocket,reuse))
            {
                CloseSocket(ThisSocket);
                return(false);
            }

            server_address=addr->CreateCopy();

            listen(ThisSocket,max_listen);

            return(true);
        }

        void ServerSocket::CloseServer()
        {
            CloseSocket(ThisSocket);
            ThisSocket=-1;
            SAFE_CLEAR(server_address);
        }

        bool ServerSocket::SetIPv6Only(bool only)
        {
            if (ThisSocket == -1)return(false);

            if (server_address->GetFamily() != AF_INET6)return(false);

            int on = only?1:0;

            return(setsockopt(ThisSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&on, sizeof(on)) == 0);
        }
    }//namespace network
}//namespace hgl
