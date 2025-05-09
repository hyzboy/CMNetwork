﻿#include<hgl/type/StrChar.h>
#include<hgl/type/String.h>
#include<hgl/io/MemoryOutputStream.h>
#include<hgl/util/hash/Hash.h>

namespace hgl
{
    namespace network
    {
        /**
         * 获取WebSocket信息
         * @param data 输入的信息头
         * @param size 信息头长度
         * @return 是否解晰成功
         */
        bool GetWebSocketInfo(U8String &sec_websocket_key,U8String &sec_websocket_protocol,uint &sec_websocket_version,const u8char *data,const uint size)
        {
            constexpr u8char SEC_WEBSOCKET_KEY[]=U8_TEXT("Sec-WebSocket-Key: ");
            constexpr uint SEC_WEBSOCKET_KEY_SIZE=sizeof(SEC_WEBSOCKET_KEY)-1;      //sizeof的带\0所以要-1

            constexpr u8char SEC_WEBSOCKET_PROTOCOL[]=U8_TEXT("Sec-WebSocket-Protocol: ");
            constexpr uint SEC_WEBSOCKET_PROTOCOL_SIZE=sizeof(SEC_WEBSOCKET_PROTOCOL)-1;

            constexpr u8char SEC_WEBSOCKET_VERSION[]=U8_TEXT("Sec-WebSocket-Version: ");
            constexpr uint SEC_WEBSOCKET_VERSION_SIZE=sizeof(SEC_WEBSOCKET_VERSION)-1;

            if(!data||size<40)return(false);

            const u8char *end;

            {
                const u8char *key=hgl::strstr(data,size,SEC_WEBSOCKET_KEY,SEC_WEBSOCKET_KEY_SIZE);

                if(!key)return(false);

                key+=SEC_WEBSOCKET_KEY_SIZE;

                end=key;
                while(*end!='\r')++end;

                sec_websocket_key=U8String(key,end-key);
            }

            {
                const u8char *protocol=hgl::strstr(data,size,SEC_WEBSOCKET_PROTOCOL,SEC_WEBSOCKET_PROTOCOL_SIZE);

                if(protocol)        //也有可能是不存在的
                {
                    protocol+=SEC_WEBSOCKET_PROTOCOL_SIZE;
                    end=protocol;
                    while(*end!='\r')++end;

                    sec_websocket_protocol.fromString(protocol,end-protocol);
                }
            }

            {
                const u8char *version=hgl::strstr(data,size,SEC_WEBSOCKET_VERSION,SEC_WEBSOCKET_VERSION_SIZE);

                if(version)
                {
                    version+=SEC_WEBSOCKET_VERSION_SIZE;
                    end=version;
                    while(*end!='\r')++end;

                    hgl::stou(version,sec_websocket_version);
                }
            }

            return(true);
        }

        ///**
        /// 这函数可以用，只是这版SDK没有base64_encode,所以暂时屏蔽
        // * 生成WebSocket回复头
        // * @param result 回复头存放字符串
        // */
        //void MakeWebSocketAccept(U8String &result,const U8String &sec_websocket_key,const U8String &sec_websocket_protocol)
        //{
        //    const U8String key_mask=sec_websocket_key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        //    util::HashCodeSHA1 hc;

        //    CountSHA1(key_mask.c_str(),key_mask.Length(),hc);

        //    io::MemoryOutputStream mos;

        //    base64_encode(&mos,hc.code,hc.size());

        //    const U8String sec_websocket_accept((char *)mos.GetData(),mos.GetSize());

        //    result="HTTP/1.1 101 Switching Protocols\r\n"
        //           "Upgrade: websocket\r\n"
        //           "Connection: Upgrade\r\n"
        //           "Sec-WebSocket-Accept: "+sec_websocket_accept;

        //    if(!sec_websocket_protocol.IsEmpty())
        //        result+="\r\nSec-WebSocket-Protocol: "+sec_websocket_protocol;

        //    result+="\r\n\r\n";
        //}
    }//namespace network
}//namespace hgl
