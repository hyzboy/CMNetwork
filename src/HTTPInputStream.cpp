#include<hgl/network/HTTPInputStream.h>
#include<hgl/network/TCPClient.h>
#include<hgl/log/log.h>
#include<hgl/type/Smart.h>

namespace hgl
{
    namespace network
    {
        namespace
        {
            constexpr char HTTP_REQUEST_HEADER_BEGIN[]= " HTTP/1.1\r\n"
                                                        "Host: ";
            constexpr uint HTTP_REQUEST_HEADER_BEGIN_SIZE=sizeof(HTTP_REQUEST_HEADER_BEGIN)-1;

            constexpr char HTTP_REQUEST_HEADER_END[]=   "\r\n"
                                                        "Accept: */*\r\n"
                                                        "User-Agent: Mozilla/5.0\r\n"
                                                        "Connection: Keep-Alive\r\n\r\n";

            constexpr uint HTTP_REQUEST_HEADER_END_SIZE=sizeof(HTTP_REQUEST_HEADER_END)-1;

            constexpr char HTTP_POST_HEADER_END[]=      "\r\n"
                                                        "Accept: */*\r\n"
                                                        "User-Agent: Mozilla/5.0\r\n"
                                                        "Content-Type: application/x-www-form-urlencoded\r\n"
                                                        "Connection: Keep-Alive\r\n"
                                                        "Content-Length: ";
            constexpr uint HTTP_POST_HEADER_END_SIZE=sizeof(HTTP_POST_HEADER_END)-1;

            constexpr uint HTTP_HEADER_BUFFER_SIZE=HGL_SIZE_1KB;
        }

        HTTPInputStream::HTTPInputStream()
        {
            tcp=nullptr;

            pos=-1;
            filelength=-1;

            tcp_is=nullptr;

            http_header=new char[HTTP_HEADER_BUFFER_SIZE];
            http_header_size=0;

            response_code=0;

            is_chunked=false;
            chunk_size=0;
            chunk_pos=0;
            chunk_header_parsed=false;
        }

        HTTPInputStream::~HTTPInputStream()
        {
            Close();
            delete[] http_header;
        }

        /**
        * 创建流并打开一个文件
        * @param host 服务器地址 www.hyzgame.org.cn 或 127.0.0.1 之类
        * @param filename 路径及文件名 /download/hgl.rar 之类
        * @return 打开文件是否成功
        */
        bool HTTPInputStream::Open(IPAddress *host_ip,const AnsiString &host_name,const AnsiString &filename)
        {
            Close();

            response_code=0;
            response_info.Clear();
            response_list.Clear();

            if(!host_ip)
                RETURN_FALSE;

            if(filename.IsEmpty())
                RETURN_FALSE;

            tcp=CreateTCPClient(host_ip);

            char *host_ip_str=host_ip->CreateString();
            SharedArray<char> self_clear(host_ip_str);

            if(!tcp)
            {
                LogError("Connect to HTTPServer failed: "+AnsiString(host_ip_str));
                RETURN_FALSE;
            }

            //设定为非堵塞模式
            tcp->SetBlock(false);

            //发送HTTP GET请求
            int len=0;

            len =strcpy(http_header    ,HTTP_HEADER_BUFFER_SIZE    ,"GET ",4);
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,filename.c_str(),           filename.Length());
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,HTTP_REQUEST_HEADER_BEGIN,  HTTP_REQUEST_HEADER_BEGIN_SIZE);
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,host_name.c_str(),          host_name.Length());
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,HTTP_REQUEST_HEADER_END,    HTTP_REQUEST_HEADER_END_SIZE);

            OutputStream *tcp_os=tcp->GetOutputStream();

            if(tcp_os->WriteFully(http_header,len)!=len)
            {
                LogError("Send HTTP Get Info failed:"+AnsiString(host_ip_str));
                delete tcp;
                tcp=nullptr;
                RETURN_FALSE;
            }

            *http_header=0;

            tcp_is=tcp->GetInputStream();
            return(true);
        }

        /**
        * POST请求打开
        * @param host_ip 服务器地址IP
        * @param host_name 服务器域名或主机名
        * @param filename 路径及资源名
        * @param post_data POST数据指针
        * @param post_data_size POST数据大小
        * @return 是否成功
        */
        bool HTTPInputStream::Post(IPAddress *host_ip,const AnsiString &host_name,const AnsiString &filename,const void *post_data,int post_data_size)
        {
            Close();

            response_code=0;
            response_info.Clear();
            response_list.Clear();

            if(!host_ip)
                RETURN_FALSE;

            if(filename.IsEmpty())
                RETURN_FALSE;

            if(!post_data||post_data_size<=0)
                RETURN_FALSE;

            tcp=CreateTCPClient(host_ip);

            char *host_ip_str=host_ip->CreateString();
            SharedArray<char> self_clear(host_ip_str);

            if(!tcp)
            {
                LogError("Connect to HTTPServer failed: "+AnsiString(host_ip_str));
                RETURN_FALSE;
            }

            //设定为非堵塞模式
            tcp->SetBlock(false);

            //发送HTTP POST请求
            int len=0;

            len =strcpy(http_header    ,HTTP_HEADER_BUFFER_SIZE    ,"POST ",5);
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,filename.c_str(),           filename.Length());
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,HTTP_REQUEST_HEADER_BEGIN,  HTTP_REQUEST_HEADER_BEGIN_SIZE);
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,host_name.c_str(),          host_name.Length());
            len+=strcpy(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,HTTP_POST_HEADER_END,       HTTP_POST_HEADER_END_SIZE);

            //追加Content-Length数值
            int content_len=snprintf(http_header+len,HTTP_HEADER_BUFFER_SIZE-len,"%d\r\n\r\n",post_data_size);
            if(content_len<=0 || len+content_len>=HTTP_HEADER_BUFFER_SIZE)
            {
                LogError("HTTP Post Header buffer overflow detected");
                delete tcp;
                tcp=nullptr;
                RETURN_FALSE;
            }
            len+=content_len;

            OutputStream *tcp_os=tcp->GetOutputStream();

            if(tcp_os->WriteFully(http_header,len)!=len)
            {
                LogError("Send HTTP Post Header failed:"+AnsiString(host_ip_str));
                delete tcp;
                tcp=nullptr;
                RETURN_FALSE;
            }

            //发送POST数据
            if(tcp_os->WriteFully(post_data,post_data_size)!=post_data_size)
            {
                LogError("Send HTTP Post Data failed:"+AnsiString(host_ip_str));
                delete tcp;
                tcp=nullptr;
                RETURN_FALSE;
            }

            *http_header=0;

            tcp_is=tcp->GetInputStream();
            return(true);
        }

        /**
        * 关闭HTTP流
        */
        void HTTPInputStream::Close()
        {
            pos=0;
            filelength=-1;

            SAFE_CLEAR(tcp);

            *http_header=0;
            http_header_size=0;

            is_chunked=false;
            chunk_size=0;
            chunk_pos=0;
            chunk_header_parsed=false;
        }

        constexpr char HTTP_HEADER_SPLITE[]="\r\n";
        constexpr uint HTTP_HEADER_SPLITE_SIZE=sizeof(HTTP_HEADER_SPLITE)-1;

        void HTTPInputStream::ParseHttpResponse()
        {
            char *offset=strstr(http_header,http_header_size,HTTP_HEADER_SPLITE,HTTP_HEADER_SPLITE_SIZE);

            if(!offset)
                return;

            response_info.fromString(http_header,offset-http_header);

            char *first=strchr(http_header,' ');

            if(!first)
                return;

            ++first;
            char *second=strchr(first,' ');

            stou(first,second-first,response_code);

            while(true)
            {
                first=offset+HTTP_HEADER_SPLITE_SIZE;

                second=strchr(first,':',http_header_size-(first-http_header));

                if(!second)break;

                AnsiString key;
                AnsiString value;

                key.fromString(first,second-first);

                first=second+2;
                second=strstr(first,http_header_size-(first-http_header),HTTP_HEADER_SPLITE,HTTP_HEADER_SPLITE_SIZE);

                if(!second)break;

                value.fromString(first,second-first);
                offset=second;

                response_list.CreateStringAttrib(key,value);
            }
        }

        constexpr char HTTP_HEADER_FINISH[]="\r\n\r\n";
        constexpr uint HTTP_HEADER_FINISH_SIZE=sizeof(HTTP_HEADER_FINISH)-1;

        constexpr char HTTP_CONTENT_LENGTH[]="Content-Length: ";
        constexpr uint HTTP_CONTENT_LENGTH_SIZE=sizeof(HTTP_CONTENT_LENGTH)-1;

        constexpr char HTTP_TRANSFER_ENCODING[]="Transfer-Encoding: ";
        constexpr uint HTTP_TRANSFER_ENCODING_SIZE=sizeof(HTTP_TRANSFER_ENCODING)-1;

        constexpr char HTTP_CHUNKED[]="chunked";
        constexpr uint HTTP_CHUNKED_SIZE=sizeof(HTTP_CHUNKED)-1;

        int HTTPInputStream::PraseHttpHeader()
        {
            char *offset;
            int size;

            offset=strstr(http_header,http_header_size,HTTP_HEADER_FINISH,HTTP_HEADER_FINISH_SIZE);

            if(!offset)
                return 0;

            ParseHttpResponse();

            *offset=0;

            size=http_header_size-(offset-http_header)-HTTP_HEADER_FINISH_SIZE;

            if(response_code==200)
            {
                // Check for Transfer-Encoding: chunked
                offset=strstr(http_header,http_header_size,HTTP_TRANSFER_ENCODING,HTTP_TRANSFER_ENCODING_SIZE);
                if(offset)
                {
                    offset+=HTTP_TRANSFER_ENCODING_SIZE;
                    char *end=strchr(offset,';');
                    if(!end)end=strchr(offset,'\r');

                    if(end && strncmp(offset,HTTP_CHUNKED,HTTP_CHUNKED_SIZE)==0)
                    {
                        is_chunked=true;
                        filelength=-1;  // Unknown size for chunked
                        LogInfo(U8_TEXT("HTTPInputStream using chunked encoding"));
                    }
                }
                else
                {
                    // Check for Content-Length
                    offset=strstr(http_header,http_header_size,HTTP_CONTENT_LENGTH,HTTP_CONTENT_LENGTH_SIZE);
                    if(offset)
                    {
                        offset+=HTTP_CONTENT_LENGTH_SIZE;
                        stou(offset,filelength);
                    }
                }

                //有些HTTP下载就是不提供文件长度

                pos=size;
                return(pos);
            }
            else
            {
                LogError("HTTPServer error info: "+AnsiString(http_header));
                return(-1);
            }
        }

        int HTTPInputStream::ReturnError()
        {
            const int err=GetLastSocketError();

            if(err==nseWouldBlock)return(0);      //不能立即完成
            if(err==0)return(0);

            LogError(OSString(OS_TEXT("Socket Error: "))+GetSocketString(err));

            Close();
            RETURN_ERROR(-2);
        }

        /**
        * 读取Chunked编码的数据
        * Chunk格式: [size in hex]\r\n[data]\r\n...0\r\n\r\n
        */
        int HTTPInputStream::ReadChunkedData(void *buf,int64 bufsize)
        {
            char *p=(char *)buf;
            int64 to_read=0;
            int readsize=0;

            while(to_read<bufsize)
            {
                // If current chunk is read completely, read next chunk header
                if(chunk_size==0 && chunk_header_parsed)
                {
                    // End of all chunks
                    if(to_read>0)return to_read;
                    return 0;
                }

                if(!chunk_header_parsed || chunk_pos>=chunk_size)
                {
                    // Read and parse chunk header
                    char chunk_header[32]={0};
                    char c;
                    int header_pos=0;

                    // Read until \r\n
                    while(header_pos<31)
                    {
                        readsize=tcp_is->Read(&c,1);
                        if(readsize<=0)return ReturnError();

                        if(c=='\r')
                        {
                            readsize=tcp_is->Read(&c,1);
                            if(readsize<=0)return ReturnError();
                            if(c=='\n')break;
                        }
                        else
                        {
                            chunk_header[header_pos++]=c;
                        }
                    }

                    chunk_header[header_pos]=0;

                    // Parse hex size
                    chunk_size=0;
                    for(int i=0;i<header_pos;i++)
                    {
                        char c=chunk_header[i];
                        chunk_size=chunk_size*16;

                        if(c>='0'&&c<='9')
                            chunk_size+=c-'0';
                        else if(c>='a'&&c<='f')
                            chunk_size+=c-'a'+10;
                        else if(c>='A'&&c<='F')
                            chunk_size+=c-'A'+10;
                    }

                    if(chunk_size==0)
                    {
                        // Last chunk, read trailing \r\n
                        tcp_is->Read(&c,1);
                        tcp_is->Read(&c,1);
                        chunk_header_parsed=true;
                        return to_read;
                    }

                    chunk_pos=0;
                    chunk_header_parsed=true;
                }

                // Read data from current chunk
                int64 left_in_chunk=chunk_size-chunk_pos;
                int64 to_copy=left_in_chunk<(bufsize-to_read)?left_in_chunk:(bufsize-to_read);

                readsize=tcp_is->Read(p,to_copy);
                if(readsize<=0)return ReturnError();

                p+=readsize;
                to_read+=readsize;
                chunk_pos+=readsize;

                // If chunk is completely read, read trailing \r\n
                if(chunk_pos>=chunk_size)
                {
                    char c;
                    tcp_is->Read(&c,1);  // \r
                    tcp_is->Read(&c,1);  // \n

                    chunk_header_parsed=false;
                }
            }

            return to_read;
        }

        /**
        * 从HTTP流中读取数据,但实际读取出来的数据长度不固定
        * @param buf 保存读出数据的缓冲区指针
        * @param bufsize 缓冲区长度,最小1024
        * @return >=0 实际读取出来的数据长度
        * @return -1 读取失败
        */
        int64 HTTPInputStream::Read(void *buf,int64 bufsize)
        {
            if(!tcp)
                RETURN_ERROR(-1);

            int readsize;

            if(response_code==0)    //HTTP头尚未解析完成
            {
                readsize=tcp_is->Read(http_header+http_header_size,HTTP_HEADER_BUFFER_SIZE-http_header_size);

                if(readsize<=0)
                    return ReturnError();

                http_header_size+=readsize;

                readsize=PraseHttpHeader();
                if(readsize==-1)
                {
                    Close();
                    RETURN_ERROR(-3);
                }

                if(pos>0)
                    memcpy(buf,http_header+http_header_size-pos,pos);

                return(pos);
            }
            else
            {
                // If chunked encoding, use special handler
                if(is_chunked)
                    return ReadChunkedData(buf,bufsize);

                readsize=tcp_is->Read((char *)buf,bufsize);

                if(readsize<=0)
                    return ReturnError();

                pos+=readsize;
                return(readsize);
            }
        }
    }//namespace network
}//namespace hgl
