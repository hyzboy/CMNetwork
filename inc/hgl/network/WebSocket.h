#pragma once

#include<hgl/type/String.h>
namespace hgl
{
    namespace network
    {
        bool GetWebSocketInfo(U8String &sec_websocket_key,U8String &sec_websocket_protocol,uint &sec_websocket_version,const u8char *data,const uint size);
        void MakeWebSocketAccept(U8String &result,const U8String &sec_websocket_key,const U8String &sec_websocket_protocol);
    }//namespace network
}//namespace hgl
