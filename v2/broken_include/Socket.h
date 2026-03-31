#pragma once    
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

namespace Socket
{
    void Setnonblock(int fd);

    class Clientsocket
    {
    public:
    public:
        int _sockfd;
        sockaddr_in _addr;
        socklen_t _len;
    };
    class ServerSocket
    {
    public:
        // Socket()
        ServerSocket(int port = 8080, const char *ip = nullptr);
        ~ServerSocket();

        int Bind();

        int Listen();

        int Accept(Socket::Clientsocket &client);

    public:
        int _sockfd;
        int _port;
        sockaddr_in _addr;
        const char *_ip;
    };
}
