#include<hgl/type/DataType.h>
#include<hgl/type/StrChar.h>

namespace hgl
{
    namespace network
    {
        namespace
        {
            struct SchemePort
            {
                uint16 port;
                char scheme[8];
            };

            const SchemePort SchemePortList[]=
            {
                {21,    "ftp"},
                {22,    "ssh"},
                {23,    "telnet"},
                {25,    "smtp"},
                {53,    "dns"},
                {80,    "http"},
                {80,    "ws"},
                {119,   "nntp"},
                {143,   "imap"},
                {389,   "ldap"},
                {443,   "https"},
                {443,   "wss"},
                {465,   "smtps"},
                {554,   "rtsp"},
                {636,   "ldaps"},
                {853,   "dnss"},
                {993,   "imaps"},
                {5060,  "sip"},
                {5061,  "sips"},
                {5222,  "xmpp"}
            };
        }

        const uint16 GetPort(const char *scheme)
        {
            for(const SchemePort &sp:SchemePortList)
                if(strcmp(scheme,sp.scheme)==0)
                    return sp.port;

            return 0;
        }
    }//namespace network
}//namespace hgl
