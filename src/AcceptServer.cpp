#include<hgl/network/AcceptServer.h>
#include<hgl/log/log.h>
#include<hgl/time/Time.h>

namespace hgl
{
    void SetTimeVal(timeval &tv,const double t_sec);

    namespace network
    {
        /**
        * 刷新服务器，并等待一个处理消息
        * @return >0 有用户接入
        * @return =0 正常，但无用户接入
        * @return <0 出错
        */
        int AcceptServer::Accept(IPAddress *addr)
        {
            if(!addr)
                return(-1);

            socklen_t sockaddr_size=server_address->GetSockAddrInSize();

            if(accept_timeout.tv_sec
             ||accept_timeout.tv_usec)
            {
                int result;

                hgl_cpy(ato,accept_timeout);            //下面的select会将数据清0,所以必须是复制一份出来用

                FD_ZERO(&accept_set);
                FD_SET(ThisSocket,&accept_set);
                result=select(ThisSocket+1,&accept_set,nullptr,nullptr,&ato);

                if(result<=0)
                    return(0);
            }

            int new_sock=accept(ThisSocket,addr->GetSockAddr(),&sockaddr_size);

            if(new_sock<0)
            {
                const int err=GetLastSocketError();

                if(err==nseTimeOut        //超时
                 ||err==nseNoError        // 0 没有错误
                 ||err==4                 //Interrupted system call(比如ctrl+c,一般DEBUG下才有)
                 ||err==11                //资源临时不可用
                 )
                    return(0);

                LogNotice(OS_TEXT("AcceptServer Accept error,errno=")+OSString::numberOf(err));

                if(err==nseTooManyLink)    //太多的人accept
                {
                    WaitTime(overload_wait);        //强制等待指定时间
                    return(0);
                }

                return(-1);
            }

            const int IP_STR_MAX_SIZE=server_address->GetIPStringMaxSize();

            // Re-allocate if size changed or first time
            if(!ipstr || ipstr_max_size < IP_STR_MAX_SIZE)
            {
                SAFE_CLEAR_ARRAY(ipstr);
                ipstr=new char[IP_STR_MAX_SIZE+1];
                ipstr_max_size=IP_STR_MAX_SIZE;
            }

            addr->ToString(ipstr,IP_STR_MAX_SIZE);

            LogInfo(U8_TEXT("AcceptServer Accept IP:")+U8String((u8char *)ipstr)+U8_TEXT(" ,sock:")+U8String::numberOf(new_sock));

            return(new_sock);
        }

        void AcceptServer::SetTimeOut(const double time_out)
        {
            SetTimeVal(accept_timeout,time_out);
        }
    }//namespace network
}//namespace hgl
