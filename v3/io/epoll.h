#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include "../broken_include/nocpy.h"

namespace servercore
{
    class Epoller : public nocopy
    {
    public:
        explicit Epoller(int max_events = 1024);
        ~Epoller();

        bool Add(int fd, uint32_t events);
        bool Mod(int fd, uint32_t events);
        bool Del(int fd);
        int Wait(int timeout_ms = -1);

        const epoll_event &GetEvent(int index) const;

    private:
        bool Control(int op, int fd, uint32_t events);

    private:
        int _epfd;
        std::vector<epoll_event> _events;
    };
}
