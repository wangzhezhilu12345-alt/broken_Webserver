#include "../broken_include/Socket.h"

void Socket::Setnonblock(int fd)
{
    // 这个函数用来设置非阻塞的fd
    int f1 = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, f1 | O_NONBLOCK);
}

Socket::ServerSocket::ServerSocket(int port, const char *ip)
    : _port(port),
      _ip(ip)
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (_sockfd < 0)
    {
        std::cerr << "socket error" << std::endl;
        exit(-1);
    }
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(_port);
    _addr.sin_addr.s_addr = _ip ? inet_addr(_ip) : INADDR_ANY;
}

int Socket::ServerSocket::Bind()
{
    int n = bind(_sockfd, (sockaddr *)&_addr, sizeof(_addr));
    if (n < 0)
    {
        return -1;
    }
    return 0;
}

int Socket::ServerSocket::Listen()
{
    int n = listen(_sockfd, 10);
    if (n < 0)
    {
        return -1;
    }
    return 0;
}

int Socket::ServerSocket::Accept(Socket::Clientsocket &client)
{
    client._len = sizeof(client._addr);
    int n = accept(_sockfd, (sockaddr *)&client._addr, &client._len);
    if (n < 0)
    {
        return -1;
    }
    client._sockfd = n;
    return 0;
}

Socket::ServerSocket::~ServerSocket()
{
    close(_sockfd);
}
