
#include "Socket.h"
#include "httpparse.h"
#include "httpresponse.h"
namespace http
{
    class Httpserver
    {
    public:
        Httpserver(int port)
            : _port(port)
        {
        }

        void Init();

        void Run();

        void do_request(Socket::Clientsocket&sock);
        void header(const HttpRequest &request, Httpresponse &response);
        void getmime(const HttpRequest &request, Httpresponse &response);
        void static_file(Httpresponse &response, const char *filebase);
        void send(const Httpresponse &response, const Socket::Clientsocket &client);

    public:
        uint16_t _port;
        Socket::ServerSocket _listensock;
    };
}
