#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

void SetNonBlock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

int main()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    SetNonBlock(listenfd);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listenfd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
    listen(listenfd, 10);

    int epollfd = epoll_create(1024);

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenfd;
    int clientfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
    epoll_event recvevents[1024];
    while (1)
    {
        int ready_num = epoll_wait(epollfd, recvevents, 1024, -1);
        for (int i = 0; i < ready_num; i++)
        {
            int currentfd = recvevents[i].data.fd;
            if (currentfd == listenfd)
            {
                while (1)
                {
                    sockaddr_in client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    clientfd = accept(listenfd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
                    if (clientfd == -1)
                    {
                        break;
                    }
                    SetNonBlock(clientfd);
                    epoll_event ev1{};
                    ev1.data.fd = clientfd;
                    ev1.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev1);
                }
                continue;
            }
            while (1)
            {
                char buffer[1024] = {0};
                ssize_t n = recv(currentfd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0)
                {
                    std::cout << "recv from fd " << currentfd << ": " << buffer << std::endl;

                    const char reply[] = "server recv ok\n";
                    send(currentfd, reply, std::strlen(reply), 0);
                    continue;
                }

                if (n == 0)
                {
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, currentfd, nullptr);
                    close(currentfd);
                    std::cout << "client closed: fd=" << currentfd << std::endl;
                }

                break;
            }
        }
    }
}
