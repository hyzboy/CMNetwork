#pragma once

#include<hgl/platform/Platform.h>
#include<hgl/type/ValueArray.h>
namespace hgl
{
    namespace network
    {
        struct SocketEvent
        {
            int sock;

            union
            {
                int size;           //数据长度(此属性为BSD系统独有)
                int error;          //错误号
            };

            bool operator==(const SocketEvent& other) const
            {
                return sock == other.sock;
            }

            bool operator!=(const SocketEvent& other) const
            {
                return !(*this == other);
            }
        };//struct SocketEvent

        using SocketEventList=ValueArray<SocketEvent>;
    }//namespace network
}//namespace hgl
