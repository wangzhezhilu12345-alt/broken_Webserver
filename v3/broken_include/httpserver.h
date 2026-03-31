#pragma once
#include "Socket.h"
#include "Threadpool.h"
#include "httpparse.h"
#include "httpresponse.h"
namespace http
{
    class Httpserver
    {
    public:
        Httpserver(int port = 8080)
            : _port(port),
              _listensock(port),
              _threadpool()
        {
        }

        void Init();

        void Run();

        void do_request(Socket::Clientsocket&sock);
        static void thread_entry(void *arg);
        void header(const HttpRequest &request, Httpresponse &response);
        void getmime(const HttpRequest &request, Httpresponse &response);
        void static_file(Httpresponse &response, const char *filebase);
        void send(const Httpresponse &response, const Socket::Clientsocket &client);

    public:
        uint16_t _port;
        Socket::ServerSocket _listensock;
        Threadpool _threadpool;
    };
}
