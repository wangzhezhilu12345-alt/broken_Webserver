#pragma once
#include "../http/httpparse.h"
#include "../broken_include/Mutexlock.h"
#include "../broken_include/nocpy.h"
#include <arpa/inet.h>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace servercore
{
    //这是IO层用来和HTTP层产生关系的对象
    class Connection
    {
    public:
        static const int BUFFER_SIZE = 4096;

        explicit Connection(int fd = -1)
            : _fd(fd),
              _addr(),
              _addr_len(0),
              _buffer(BUFFER_SIZE, 0),
              _read_index(0),
              _check_index(0),
              _start_line(0),
              _parse_state(http::HttpRequestParse::PARSE_REQUESTLINE),
              _processing(false),
              _registered_in_epoll(false),
              _write_offset(0),
              _write_retry_count(0),
              _next_retry_ms(0)
        {
        }

    public:
        int _fd;
        sockaddr_in _addr;
        socklen_t _addr_len;
        Mutexlock _mutex;
        std::vector<char> _buffer;
        int _read_index;
        int _check_index;
        int _start_line;
        http::HttpRequestParse::PARSE_STATUS _parse_state;
        http::HttpRequest _request;
        bool _processing;
        bool _registered_in_epoll;
        std::string _pending_response;
        size_t _write_offset;
        int _write_retry_count;
        uint64_t _next_retry_ms;
    };

    class ConnectionStore : public nocopy
    {
    public:
        std::shared_ptr<Connection> Add(int fd, const sockaddr_in &addr, socklen_t addr_len);
        std::shared_ptr<Connection> Get(int fd);
        void Remove(int fd);
        std::vector<std::shared_ptr<Connection> > Snapshot();

    private:
        Mutexlock _mutex;
        std::unordered_map<int, std::shared_ptr<Connection> > _connections;
    };
}
