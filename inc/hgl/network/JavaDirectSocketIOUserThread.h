#pragma once

#include<hgl/network/DirectSocketIOUserThread.h>
#include<hgl/io/JavaInputStream.h>
#include<hgl/io/JavaOutputStream.h>

namespace hgl
{
    using namespace hgl::io;

    namespace network
    {
        typedef DirectSocketIOUserThread<JavaInputStream,JavaOutputStream> JavaDirectSocketIOUserThread;
    }//namespace network
}//namespace hgl
