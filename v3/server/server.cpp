#include "server.h"
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sys/signalfd.h>
#include <sys/timerfd.h>

namespace
{
    const int BUFFER_SIZE = servercore::Connection::BUFFER_SIZE;
}

serverapp::Server::Server(int port)
    : _port(port),
      _listensock(port),
      _epoller(),
      _signal_fd(-1),
      _timer_fd(-1),
      _running(true),
      _connections(),
      _logic_requests(),
      _logic_results(),
      _logic(_logic_requests, _logic_results),
      _page_service(),
      _http_handler()
{
}

void serverapp::Server::Init()
{
    //这是设计了逻辑层的处理函数...
    _logic.SetHandler([this](const servercore::LogicRequest &request) {
        servercore::LogicResult result;
        result._connection = request._connection;
        result._request = request._request;
        result._page = _page_service.Resolve(request._request, ".");
        return result;
    });

    if (!_page_service.Init())
    {
        std::cerr << "warning: failed to initialize MySQL login storage" << std::endl;
    }

    if (_listensock.Bind() < 0)
    {
        perror("bind error");
    }
    if (_listensock.Listen() < 0)
    {
        perror("listen error");
    }

    Socket::Setnonblock(_listensock._sockfd);
    if (!_epoller.Add(_listensock._sockfd, EPOLLIN))
    {
        perror("epoll add listenfd error");
    }

    //这是信号的初始化函数
    InitSignalEvent();
    //这是计时器的初始化函数
    InitTimerEvent();
}

void serverapp::Server::Run()
{
    while (_running)
    {
        DrainLogicResults();

        int event_count = _epoller.Wait(100);
        if (event_count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("epoll wait error");
            continue;
        }

        for (int i = 0; i < event_count; ++i)
        {
            const epoll_event &event = _epoller.GetEvent(i);
            int fd = event.data.fd;

            if (fd == _signal_fd)
            {
                HandleSignalEvent();
                continue;
            }

            if (fd == _timer_fd)
            {
                HandleTimerEvent();
                continue;
            }

            if (fd == _listensock._sockfd)
            {
                AcceptConnections();
                continue;
            }

            if ((event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) != 0)
            {
                CloseConnection(fd);
                continue;
            }

            if ((event.events & EPOLLIN) != 0)
            {
                DispatchConnection(fd);
            }

            if ((event.events & EPOLLOUT) != 0)
            {
                FlushConnection(fd);
            }
        }

        DrainLogicResults();
    }
}

void serverapp::Server::InitSignalEvent()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) < 0)
    {
        perror("sigprocmask error");
        return;
    }

    //注册一个信号fd，这是一种统一事件源的思想
    _signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (_signal_fd < 0)
    {
        perror("signalfd error");
        return;
    }

    if (!_epoller.Add(_signal_fd, EPOLLIN))
    {
        perror("epoll add signalfd error");
        close(_signal_fd);
        _signal_fd = -1;
    }
}

void serverapp::Server::InitTimerEvent()
{
    //同上
    _timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (_timer_fd < 0)
    {
        perror("timerfd_create error");
        return;
    }


    //要设置一个触发周期
    itimerspec timer_spec;
    std::memset(&timer_spec, 0, sizeof(timer_spec));
    timer_spec.it_interval.tv_sec = WRITE_RETRY_INTERVAL_MS / 1000;
    timer_spec.it_interval.tv_nsec = (WRITE_RETRY_INTERVAL_MS % 1000) * 1000000;
    timer_spec.it_value = timer_spec.it_interval;

    if (timerfd_settime(_timer_fd, 0, &timer_spec, nullptr) < 0)
    {
        perror("timerfd_settime error");
        close(_timer_fd);
        _timer_fd = -1;
        return;
    }

    //把这个扔到统一的事件循环
    if (!_epoller.Add(_timer_fd, EPOLLIN))
    {
        perror("epoll add timerfd error");
        close(_timer_fd);
        _timer_fd = -1;
    }
}

void serverapp::Server::HandleSignalEvent()
{
    if (_signal_fd < 0)
    {
        return;
    }

    while (true)
    {
        signalfd_siginfo signal_info;
        ssize_t n = read(_signal_fd, &signal_info, sizeof(signal_info));
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            perror("read signalfd error");
            break;
        }
        if (n == 0)
        {
            break;
        }

        if (signal_info.ssi_signo == SIGINT || signal_info.ssi_signo == SIGTERM)
        {
            std::cerr << "receive signal " << signal_info.ssi_signo << ", stop server" << std::endl;
            _running = false;
        }
    }
}

void serverapp::Server::HandleTimerEvent()
{
    if (_timer_fd < 0)
    {
        return;
    }

    uint64_t expirations = 0;
    while (true)
    {
        ssize_t n = read(_timer_fd, &expirations, sizeof(expirations));
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            perror("read timerfd error");
            return;
        }
        break;
    }

    uint64_t now = NowMs();
    std::vector<std::shared_ptr<servercore::Connection> > snapshot = _connections.Snapshot();
    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        const std::shared_ptr<servercore::Connection> &connection = snapshot[i];
        bool should_retry = false;
        bool should_close = false;
        {
            MutexlockGuard lock(connection->_mutex);
            if (connection->_pending_response.empty() ||
                connection->_write_offset >= connection->_pending_response.size())
            {
                continue;
            }

            if (connection->_next_retry_ms > now)
            {
                continue;
            }

            if (connection->_write_retry_count >= MAX_WRITE_RETRY)
            {
                should_close = true;
            }
            else
            {
                ++connection->_write_retry_count;
                connection->_next_retry_ms = now + WRITE_RETRY_INTERVAL_MS;
                should_retry = true;
            }
        }

        if (should_close)
        {
            CloseConnection(connection->_fd);
            continue;
        }

        if (should_retry)
        {
            FlushConnection(connection->_fd);
        }
    }
}

void serverapp::Server::AcceptConnections()
{
    while (true)
    {
        Socket::Clientsocket client;
        if (_listensock.Accept(client) < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            perror("accept error");
            break;
        }

        Socket::Setnonblock(client._sockfd);
        std::shared_ptr<servercore::Connection> connection =
            _connections.Add(client._sockfd, client._addr, client._len);
        UpdateConnectionEvents(connection, EPOLLIN | EPOLLET | EPOLLRDHUP);
    }
}

void serverapp::Server::DispatchConnection(int fd)
{
    std::shared_ptr<servercore::Connection> connection = _connections.Get(fd);
    if (!connection)
    {
        return;
    }

    if (!MarkConnectionProcessing(connection, true))
    {
        return;
    }

    _epoller.Del(fd);
    {
        MutexlockGuard lock(connection->_mutex);
        connection->_registered_in_epoll = false;
    }

    if (!ReadFromConnection(connection))
    {
        CloseConnection(fd);
        return;
    }

    http::HttpRequestParse::HTTP_CODE code = _http_handler.ParseRequest(connection);

    if (code == http::HttpRequestParse::NO_REQUEST)
    {
        RearmConnection(fd);
        return;
    }

    if (code != http::HttpRequestParse::GET_REQUEST)
    {
        QueueResponse(connection, _http_handler.BuildSimpleResponseData("400 Bad Request", "Bad Request"));
        return;
    }

    servercore::LogicRequest request;
    request._connection = connection;
    {
        MutexlockGuard lock(connection->_mutex);
        request._request = connection->_request;
    }

    if (!_logic_requests.Push(request))
    {
        QueueResponse(connection, _http_handler.BuildSimpleResponseData("503 Service Unavailable", "Server Busy"));
    }
}

void serverapp::Server::FlushConnection(int fd)
{
    std::shared_ptr<servercore::Connection> connection = _connections.Get(fd);
    if (!connection)
    {
        return;
    }

    if (!TryFlushPendingResponse(connection))
    {
        CloseConnection(fd);
    }
}

void serverapp::Server::DrainLogicResults()
{
    //把逻辑层已经处理完的结果，从结果队列里面拿出来再交付给主线程处理
    servercore::LogicResult result;
    while (_logic_results.TryPop(result))
    {
        HandleLogicResult(result);
    }
}

void serverapp::Server::HandleLogicResult(const servercore::LogicResult &result)
{
    if (!result._connection)
    {
        return;
    }

    http::Httpresponse response;
    _http_handler.BuildResponse(result._request, result._page, response);

    std::string response_data;
    if (!_http_handler.BuildResponseData(response, response_data))
    {
        response_data = _http_handler.BuildSimpleResponseData("500 Internal Server Error", "error");
    }

    QueueResponse(result._connection, response_data);
}

void serverapp::Server::QueueResponse(const std::shared_ptr<servercore::Connection> &connection, const std::string &response)
{
    {
        MutexlockGuard lock(connection->_mutex);
        connection->_pending_response = response;
        connection->_write_offset = 0;
        connection->_write_retry_count = 0;
        connection->_next_retry_ms = NowMs() + WRITE_RETRY_INTERVAL_MS;
        connection->_processing = false;
    }

    if (!TryFlushPendingResponse(connection))
    {
        CloseConnection(connection->_fd);
    }
}

bool serverapp::Server::TryFlushPendingResponse(const std::shared_ptr<servercore::Connection> &connection)
{
    while (true)
    {
        std::string pending;
        size_t offset = 0;
        {
            MutexlockGuard lock(connection->_mutex);
            if (connection->_pending_response.empty())
            {
                UpdateConnectionEvents(connection, EPOLLIN | EPOLLET | EPOLLRDHUP);
                return true;
            }

            pending = connection->_pending_response;
            offset = connection->_write_offset;
        }

        ssize_t n = send(connection->_fd,
                         pending.data() + offset,
                         pending.size() - offset,
                         MSG_NOSIGNAL);
        if (n > 0)
        {
            bool finished = false;
            {
                MutexlockGuard lock(connection->_mutex);
                connection->_write_offset += static_cast<size_t>(n);
                connection->_write_retry_count = 0;
                connection->_next_retry_ms = NowMs() + WRITE_RETRY_INTERVAL_MS;
                finished = connection->_write_offset >= connection->_pending_response.size();
                if (finished)
                {
                    connection->_pending_response.clear();
                    connection->_write_offset = 0;
                }
            }

            if (finished)
            {
                CloseConnection(connection->_fd);
                return true;
            }
            continue;
        }

        if (n == 0)
        {
            return false;
        }

        if (errno == EINTR)
        {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            UpdateConnectionEvents(connection, EPOLLOUT | EPOLLET | EPOLLRDHUP);
            return true;
        }

        return false;
    }
}

void serverapp::Server::UpdateConnectionEvents(const std::shared_ptr<servercore::Connection> &connection, uint32_t events)
{
    bool registered = false;
    {
        MutexlockGuard lock(connection->_mutex);
        registered = connection->_registered_in_epoll;
    }

    bool ok = registered ? _epoller.Mod(connection->_fd, events) : _epoller.Add(connection->_fd, events);
    if (!ok)
    {
        CloseConnection(connection->_fd);
        return;
    }

    MutexlockGuard lock(connection->_mutex);
    connection->_registered_in_epoll = true;
}

void serverapp::Server::RearmConnection(int fd)
{
    std::shared_ptr<servercore::Connection> connection = _connections.Get(fd);
    if (!connection)
    {
        return;
    }
    MarkConnectionProcessing(connection, false);
    UpdateConnectionEvents(connection, EPOLLIN | EPOLLET | EPOLLRDHUP);
}

void serverapp::Server::CloseConnection(int fd)
{
    std::shared_ptr<servercore::Connection> connection = _connections.Get(fd);
    if (connection)
    {
        MutexlockGuard lock(connection->_mutex);
        connection->_processing = false;
        connection->_registered_in_epoll = false;
    }
    _epoller.Del(fd);
    _connections.Remove(fd);
    close(fd);
}

bool serverapp::Server::ReadFromConnection(const std::shared_ptr<servercore::Connection> &connection)
{
    while (true)
    {
        MutexlockGuard lock(connection->_mutex);
        if (connection->_read_index >= BUFFER_SIZE)
        {
            return false;
        }

        ssize_t recvdata = recv(connection->_fd,
                                connection->_buffer.data() + connection->_read_index,
                                BUFFER_SIZE - connection->_read_index,
                                0);
        if (recvdata > 0)
        {
            connection->_read_index += static_cast<int>(recvdata);
            continue;
        }

        if (recvdata == 0)
        {
            return false;
        }

        if (errno == EINTR)
        {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return true;
        }

        return false;
    }
}

bool serverapp::Server::MarkConnectionProcessing(const std::shared_ptr<servercore::Connection> &connection, bool processing)
{
    MutexlockGuard lock(connection->_mutex);
    if (processing && connection->_processing)
    {
        return false;
    }
    connection->_processing = processing;
    return true;
}

uint64_t serverapp::Server::NowMs() const
{
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}
