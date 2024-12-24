#include<hgl/network/UserAgentString.h>

namespace hgl
{
    namespace webapi
    {
        U8String FirefoxUserAgent(FirefoxUserAgentConfig &cfg)
        {
            U8String agent=U8_TEXT("Mozilla/5.0 ");

            if(cfg.os>=OS_WindowsX86
             &&cfg.os<=OS_LinuxX86_64)
            {
                if(cfg.os>=OS_WindowsX86
                 &&cfg.os<=OS_WindowsWOW64)
                {
                    agent+=U8_TEXT("(Windows NT ")+U8String::numberOf(cfg.os_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.os_ver.minor)+U8_TEXT("; ");

                    if(cfg.os==OS_WindowsAMD64)agent+=U8_TEXT("Win64; x64; ");else
                    if(cfg.os==OS_WindowsWOW64)agent+=U8_TEXT("WOW64; ");
                }
                else
                if(cfg.os==OS_macOS)
                    agent+=U8_TEXT("(Macintosh; Intel Mac OS X ")+U8String::numberOf(cfg.os_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.os_ver.minor)+U8_TEXT("; ");
                else
                if(cfg.os==OS_Linuxi686)
                    agent+=U8_TEXT("(X11; Linux i686; ");
                else
                if(cfg.os==OS_LinuxX86_64)
                    agent+=U8_TEXT("(X11; Linux x86_64; ");

                agent+=U8_TEXT("rv:")+U8String::numberOf(cfg.ff_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.ff_ver.minor)+U8_TEXT(") Gecko/")+U8String::numberOf(cfg.gecko_version)+U8_TEXT(" Firefox/")
                                     +U8String::numberOf(cfg.ff_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.ff_ver.minor);
            }
            else
            if(cfg.os>=OS_iPod
             &&cfg.os<=OS_iPad)
            {
                if(cfg.os==OS_iPod) agent+=U8_TEXT("(iPod touch; ");else
                if(cfg.os==OS_iPad) agent+=U8_TEXT("(iPad; ");else
                                    agent+=U8_TEXT("(iPhone; ");

                agent+= U8_TEXT("CPU iPhone OS ")+U8String::numberOf(cfg.os_ver.major)+U8_TEXT("_")+U8String::numberOf(cfg.os_ver.minor)+U8_TEXT(" like Mac OS X) ")+
                        U8_TEXT("AppleWebKit/600.1.4 (KHTML, like Gecko) FxiOS/1.0 Mobile/12F69 Safari/600.1.4");
            }
            else
            if(cfg.os>=OS_AndroidPhone
             &&cfg.os<=OS_AndroidTV)
            {
                agent+=U8_TEXT("(Android ")+U8String::numberOf(cfg.os_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.os_ver.minor)+U8_TEXT("; ");

                if(cfg.os==OS_AndroidPhone  )agent+=U8_TEXT("Mobile; ");
                if(cfg.os==OS_AndroidTablet )agent+=U8_TEXT("Tablet; ");
                if(cfg.os==OS_AndroidTV     )agent+=U8_TEXT("TV; ");

                agent+=U8_TEXT("rv:")+U8String::numberOf(cfg.ff_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.ff_ver.minor)+U8_TEXT(") Gecko/")
                                     +U8String::numberOf(cfg.ff_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.ff_ver.minor)+U8_TEXT(" Firefox/")
                                     +U8String::numberOf(cfg.ff_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.ff_ver.minor);
            }
            else
            {
                agent+=U8_TEXT("Firefox/")+U8String::numberOf(cfg.ff_ver.major)+U8_TEXT(".")+U8String::numberOf(cfg.ff_ver.minor);
            }

            return agent;
        }
    }//namespace webapi
}//namespace hgl
