#include<hgl/network/TCPAccept.h>
#include<hgl/network/SocketInputStream.h>
#include<hgl/network/SocketOutputStream.h>
#include<hgl/io/DataInputStream.h>
#include<hgl/io/DataOutputStream.h>
#include<hgl/type/StrChar.h>

namespace hgl
{
    namespace network
    {
        bool TCPAccept::Send(void *data,const uint size)
        {
            if(!data)return(false);
            if(size<=0)return(false);

            if(!output_stream_)
                output_stream_ = std::make_unique<SocketOutputStream>(GetSocket());

            int result=output_stream_->WriteFully(data,size);

            if(result!=size)
                return(false);

            return(true);
        }
    }//namespace network
}//namespace hgl
