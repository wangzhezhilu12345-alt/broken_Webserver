#include "epoll.h"
#include <cstring>

servercore::Epoller::Epoller(int max_events)
    : _epfd(epoll_create1(0)),
      _events(max_events)
{
}

servercore::Epoller::~Epoller()
{
    if (_epfd >= 0)
    {
        close(_epfd);
    }
}

bool servercore::Epoller::Add(int fd, uint32_t events)
{
    return Control(EPOLL_CTL_ADD, fd, events);
}

bool servercore::Epoller::Mod(int fd, uint32_t events)
{
    return Control(EPOLL_CTL_MOD, fd, events);
}

bool servercore::Epoller::Del(int fd)
{
    return Control(EPOLL_CTL_DEL, fd, 0);
}

int servercore::Epoller::Wait(int timeout_ms)
{
    return epoll_wait(_epfd, _events.data(), static_cast<int>(_events.size()), timeout_ms);
}

const epoll_event &servercore::Epoller::GetEvent(int index) const
{
    return _events[index];
}

bool servercore::Epoller::Control(int op, int fd, uint32_t events)
{
    epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = events;
    return epoll_ctl(_epfd, op, fd, op == EPOLL_CTL_DEL ? nullptr : &event) == 0;
}
