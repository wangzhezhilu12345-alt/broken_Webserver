#include "../broken_include/httpserver.h"
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
using namespace http;
const int BUFFERSIZE = 4096;

namespace
{
    struct RequestTask
    {
        Httpserver *server;
        Socket::Clientsocket client;
    };

    bool send_all(int fd, const void *data, size_t len)
    {
        const char *p = static_cast<const char *>(data);
        size_t sent = 0;
        while (sent < len)
        {
            ssize_t n = ::send(fd, p + sent, len - sent, 0);
            if (n < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return false;
            }
            if (n == 0)
            {
                return false;
            }
            sent += static_cast<size_t>(n);
        }
        return true;
    }
}

void http::Httpserver::Init()
{
    int n1 = _listensock.Bind();
    if (n1 < 0)
    {
        perror("bind error!");
    }
    _listensock.Listen();
}
void http::Httpserver::Run()
{
    while (1)
    {
        Socket::Clientsocket clientsocket;
        int ret = _listensock.Accept(clientsocket);
        if (ret < 0)
        {
            continue;
        }

        //本次请求的数据，一个是这个服务器，一个是用户的fd
        RequestTask *task_arg = new RequestTask{this, clientsocket};
        if (!_threadpool.PushTask(Task(Httpserver::thread_entry, task_arg)))
        {
            close(clientsocket._sockfd);
            delete task_arg;
        }
    }
}

void http::Httpserver::thread_entry(void *arg)
{
    RequestTask *task = static_cast<RequestTask *>(arg);
    task->server->do_request(task->client);
    close(task->client._sockfd);
    delete task;
}

void Httpserver::do_request(Socket::Clientsocket &clientsock)
{
    char buffer[BUFFERSIZE];
    bzero(buffer, sizeof(buffer));
    // 先更新状态机
    http::HttpRequestParse::PARSE_STATUS status = http::HttpRequestParse::PARSE_REQUESTLINE;
    ssize_t recvdata = 0;
    http::HttpRequest request;
    int read_index = 0, check_index = 0, start_line = 0;
    while (1)
    {
        recvdata = recv(clientsock._sockfd, buffer + read_index, BUFFERSIZE - read_index, 0);
        if (recvdata < 0)
        {
            perror("recv error");
            exit(0);
        }
        else if (recvdata == 0)
        {
            perror("connection is closed by peer");
            break;
        }

        read_index += recvdata;
        if (read_index >= BUFFERSIZE)
        {
            const char *err =
                "HTTP/1.1 413 Payload Too Large\r\n"
                "Connection: close\r\n"
                "Content-Length: 0\r\n\r\n";
            send_all(clientsock._sockfd, err, strlen(err));
            break;
        }

        // 确定收到了请求之后，就进行解析
        http::HttpRequestParse::HTTP_CODE recvmsg = http::HttpRequestParse::parse_content(buffer, check_index, read_index, status, start_line, request);
        if (recvmsg == http::HttpRequestParse::NO_REQUEST)
        {
            printf("recvmsg = %d\n", recvmsg);
            continue;
        }

        if (recvmsg == http::HttpRequestParse::GET_REQUEST)
        {
            // 如果是GET请求的话就处理
            http::Httpresponse reponse;

            // 下面是http的业务逻辑处理
            header(request, reponse);
            getmime(request, reponse);
            static_file(reponse, "."); // 以当前工作目录为站点根目录
            send(reponse, clientsock);
            break;
        }
        else
        {
            // 就输出一个错误，暂时没想好post请求怎么处理
            std::cout << "Bad Request" << std::endl;
        }
    }
}
// 这个函数我在全局定义的
void http::Httpserver::header(const http::HttpRequest &a, http::Httpresponse &b)
{
    if (a._Version == HttpRequest::HTTP_10)
        b._Version = http::HttpRequest::HTTP_10;
    else
        b._Version = http::HttpRequest::HTTP_11;

    // 加头？
    b._header["Broken"] = "Broken Server";
}

void http::Httpserver::getmime(const http::HttpRequest &a, http::Httpresponse &b)
{
    int pos = 0;
    std::string filepath = a._Url;
    if (filepath == "/")
    {
        filepath = "/index.html";
    }
    if ((pos = filepath.rfind('?')) != std::string::npos)
    {
        filepath.erase(filepath.rfind('?'));
    }
    std::string mime;
    if (filepath.rfind('.') != std::string::npos)
    {
        mime = filepath.substr(filepath.rfind('.'));
    }
    decltype(http::mime_map)::iterator it;
    if ((it = http::mime_map.find(mime)) != http::mime_map.end())
    {
        b.mime = it->second;
    }
    b._filepath = filepath;
}
void http::Httpserver::static_file(http::Httpresponse &a, const char *filebase)
{
    std::string file = std::string(filebase) + a._filepath;
    struct stat file_stat;
    if (stat(file.c_str(), &file_stat) < 0)
    {
        a.Statuscode = http::Httpresponse::NOTFOUND;
        a.statusmsg = "Not Found";
        a._filepath = file;
        return;
    }

    if (!S_ISREG(file_stat.st_mode))
    {
        a.Statuscode = http::Httpresponse::Frobidden403;
        a.statusmsg = "Frobidden";
        a._filepath = file;
        return;
    }

    a.Statuscode = http::Httpresponse::OK;
    a.statusmsg = "OK";
    a._filepath = file;
}

void http::Httpserver::send(const http::Httpresponse &respone, const Socket::Clientsocket &client)
{
    struct stat file_stat;

    if (stat(respone._filepath.c_str(), &file_stat) < 0) // 处理错误
    {
        std::string err =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found";

        send_all(client._sockfd, err.c_str(), err.size());
        return;
    }

    int filefd = ::open(respone._filepath.c_str(), O_RDONLY);
    if (filefd < 0)
    {
        std::string err =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "error";
        send_all(client._sockfd, err.c_str(), err.size());
        return;
    }

    std::string header;
    header += "HTTP/1.1 ";
    header += std::to_string(static_cast<int>(respone.Statuscode));
    header += " ";
    header += respone.statusmsg;
    header += "\r\n";
    header += "Connection: close\r\n";
    header += "Content-Length: " + std::to_string(file_stat.st_size) + "\r\n";
    header += "Content-Type: " + respone.mime.type + "\r\n";
    header += "\r\n";
    if (!send_all(client._sockfd, header.c_str(), header.size()))
    {
        close(filefd);
        return;
    }

    void *mapbuf = mmap(nullptr, file_stat.st_size, PROT_READ, MAP_PRIVATE, filefd, 0);
    if (mapbuf == MAP_FAILED)
    {
        perror("mmap");

        std::string err =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "error";

        send_all(client._sockfd, err.c_str(), err.size());
        close(filefd);
        return;
    }

    send_all(client._sockfd, mapbuf, file_stat.st_size);
    munmap(mapbuf, file_stat.st_size);
    close(filefd);
}
