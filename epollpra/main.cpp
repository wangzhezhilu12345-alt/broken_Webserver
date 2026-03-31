#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
constexpr int kPort = 8080;
constexpr int kMaxEvents = 16;
constexpr int kBufferSize = 1024;

int CreateListenFd()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        std::perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::perror("setsockopt");
        close(listenfd);
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(kPort);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1)
    {
        std::perror("bind");
        close(listenfd);
        return -1;
    }

    if (listen(listenfd, 10) == -1)
    {
        std::perror("listen");
        close(listenfd);
        return -1;
    }

    return listenfd;
}

int AddFdToEpoll(int epollfd, int fd)
{
    epoll_event ev{};//结构体设置一下内容
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    return epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}
}

int main()
{
    int listenfd = CreateListenFd();
    if (listenfd == -1)
    {
        return 1;
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        std::perror("epoll_create1");
        close(listenfd);
        return 1;
    }

    if (AddFdToEpoll(epollfd, listenfd) == -1)
    {
        std::perror("epoll_ctl listenfd");
        close(epollfd);
        close(listenfd);
        return 1;
    }

    std::cout << "epoll server is listening on 0.0.0.0:" << kPort << std::endl;

    epoll_event events[kMaxEvents];
    while (true)
    {
        int ready_num = epoll_wait(epollfd, events, kMaxEvents, -1);
        if (ready_num == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            std::perror("epoll_wait");
            break;
        }

        for (int i = 0; i < ready_num; ++i)
        {
            int current_fd = events[i].data.fd;

            if (current_fd == listenfd)
            {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int clientfd = accept(listenfd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
                if (clientfd == -1)
                {
                    std::perror("accept");
                    continue;
                }

                if (AddFdToEpoll(epollfd, clientfd) == -1)
                {
                    std::perror("epoll_ctl clientfd");
                    close(clientfd);
                    continue;
                }

                std::cout << "new client: " << inet_ntoa(client_addr.sin_addr) << ":"
                          << ntohs(client_addr.sin_port) << std::endl;
                continue;
            }

            char buffer[kBufferSize] = {0};
            ssize_t n = recv(current_fd, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0)
            {
                if (n < 0)
                {
                    std::perror("recv");
                }
                epoll_ctl(epollfd, EPOLL_CTL_DEL, current_fd, nullptr);
                close(current_fd);
                std::cout << "client closed: fd=" << current_fd << std::endl;
                continue;
            }

            std::cout << "recv from fd " << current_fd << ": " << buffer << std::endl;

            const char reply[] = "server recv ok\n";
            send(current_fd, reply, std::strlen(reply), 0);
        }
    }

    close(epollfd);
    close(listenfd);
    return 0;
}
