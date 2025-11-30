#include"SocketManageBase.h"
#include<hgl/network/Socket.h>
#include<hgl/log/Log.h>

#include<unistd.h>
#include<sys/types.h>
#include<sys/event.h>
#include<sys/time.h>

namespace hgl
{
    namespace network
    {
        class SocketManageKqueue:public SocketManageBase
        {
            OBJECT_LOGGER

        protected:

            int kqueue_fd;
            uint user_event;

            int max_connect;
            int cur_count;

        protected:

            struct kevent *event_list;

        private:

            bool kqueue_add(int sock)
            {
                struct kevent ev[2];
                int nev = 0;

                // Add read event
                EV_SET(&ev[nev], sock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)sock);
                nev++;

                // Add write event (optional, similar to epoll EPOLLOUT)
                // Comment out if only interested in recv like in epoll version
                // EV_SET(&ev[nev], sock, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)sock);
                // nev++;

                return(kevent(kqueue_fd, ev, nev, nullptr, 0, nullptr) == 0);
            }

            bool kqueue_del(int sock)
            {
                struct kevent ev[2];
                int nev = 0;

                // Remove read event
                EV_SET(&ev[nev], sock, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
                nev++;

                // Remove write event if it was added
                // EV_SET(&ev[nev], sock, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
                // nev++;

                return(kevent(kqueue_fd, ev, nev, nullptr, 0, nullptr) == 0);
            }

        public:

            SocketManageKqueue(int kqfd, uint ue, int mc)
            {
                kqueue_fd = kqfd;
                user_event = ue;

                max_connect = mc;
                cur_count = 0;

                event_list = new struct kevent[max_connect];
            }

            ~SocketManageKqueue()
            {
                delete[] event_list;

                close(kqueue_fd);
            }

            bool Join(int sock) override
            {
                kqueue_add(sock);

                SetSocketBlock(sock, false);

                ++cur_count;

                LogInfo(OS_TEXT("SocketManageKqueue::Join() Socket:"), OSString(sock));

                return(true);
            }

            bool Unjoin(int sock) override
            {
                if(kqueue_fd == -1)
                {
                    LogError(OS_TEXT("SocketManageKqueue::Unjoin() kqueue_fd==-1)"));
                    return(false);
                }

                --cur_count;
                kqueue_del(sock);

                LogInfo(OS_TEXT("SocketManageKqueue::Unjoin() Socket:"), OSString(sock));

                return(true);
            }

            int GetCount()const override
            {
                return cur_count;
            }

            void Clear() override
            {
                if(kqueue_fd != -1)
                {
                    close(kqueue_fd);
                    kqueue_fd = -1;
                }

                cur_count = 0;
            }

            int Update(const double &time_out, SocketEventList &recv_list, SocketEventList &send_list, SocketEventList &error_list) override
            {
                int event_count = 0;

                if(kqueue_fd == -1)
                    return(-1);

                if(cur_count <= 0)
                    return(0);

                struct timespec ts, *tsp = nullptr;

                if(time_out > 0)
                {
                    ts.tv_sec = (time_t)time_out;
                    ts.tv_nsec = (long)((time_out - ts.tv_sec) * 1000000000);
                    tsp = &ts;
                }

                event_count = kevent(kqueue_fd, nullptr, 0, event_list, cur_count, tsp);

                if(event_count == 0)
                    return(0);

                if(event_count < 0)
                {
                    LogInfo(OS_TEXT("kevent return -1,errno: "), OSString(errno));

                    if(errno == EBADF
                     ||errno == EFAULT
                     ||errno == EINVAL)
                        return(-1);

                    return(0);
                }

                recv_list.PreMalloc(event_count);
                send_list.PreMalloc(event_count);
                error_list.PreMalloc(event_count);

                struct kevent *ee = this->event_list;

                SocketEvent *rp = recv_list.GetData();
                SocketEvent *sp = send_list.GetData();
                SocketEvent *ep = error_list.GetData();

                int recv_num = 0;
                int send_num = 0;
                int error_num = 0;

                for(int i = 0; i < event_count; i++)
                {
                    if(ee->flags & EV_ERROR)
                    {
                        LogError("SocketManageKqueue Error,socket:", OSString((int)(intptr_t)ee->udata), ",kevent error:", OSString(ee->data));

                        ep->sock = (int)(intptr_t)ee->udata;
                        ep->error = ee->data;
                        ++ep;
                        ++error_num;
                    }
                    else if(ee->filter == EVFILT_READ)
                    {
                        rp->sock = (int)(intptr_t)ee->udata;
                        rp->size = 0;
                        ++rp;
                        ++recv_num;
                    }
                    else if(ee->filter == EVFILT_WRITE)
                    {
                        sp->sock = (int)(intptr_t)ee->udata;
                        sp->size = 0;
                        ++sp;
                        ++send_num;
                    }

                    ++ee;
                }

                recv_list.SetCount(recv_num);
                send_list.SetCount(send_num);
                error_list.SetCount(error_num);

                return(event_count);
            }
        };//class SocketManageKqueue:public SocketManageBase

        SocketManageBase *CreateSocketManageBase(int max_user)
        {
            if(max_user <= 0)
                return(nullptr);

            int kqueue_fd = kqueue();

            if(kqueue_fd < 0)
            {
                GLogError(OS_TEXT("kqueue() return error,errno is"), OSString(errno));
                return(nullptr);
            }

            return(new SocketManageKqueue(kqueue_fd, 0, max_user));
        }
    }//namespace network
}//namespace hgl

