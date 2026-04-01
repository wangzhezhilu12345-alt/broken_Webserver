#pragma once
#include "../http/http_handler.h"
#include "../io/Socket.h"
#include "../io/connection.h"
#include "../io/epoll.h"
#include "../logic/logic.h"
#include "../logic/page_service.h"
#include "../logic/request_queue.h"
#include <memory>
#include <string>
#include <stdint.h>

namespace serverapp
{
    class Server
    {
    public:
        explicit Server(int port = 8080);

        void Init();
        void Run();

    private:
        void AcceptConnections();
        void DispatchConnection(int fd);
        void FlushConnection(int fd);
        void DrainLogicResults();
        void HandleLogicResult(const servercore::LogicResult &result);
        void InitSignalEvent();
        void InitTimerEvent();
        void HandleSignalEvent();
        void HandleTimerEvent();
        void QueueResponse(const std::shared_ptr<servercore::Connection> &connection, const std::string &response);
        bool TryFlushPendingResponse(const std::shared_ptr<servercore::Connection> &connection);
        void UpdateConnectionEvents(const std::shared_ptr<servercore::Connection> &connection, uint32_t events);
        void RearmConnection(int fd);
        void CloseConnection(int fd);
        bool ReadFromConnection(const std::shared_ptr<servercore::Connection> &connection);
        bool MarkConnectionProcessing(const std::shared_ptr<servercore::Connection> &connection, bool processing);
        uint64_t NowMs() const;

    private:
        uint16_t _port;
        Socket::ServerSocket _listensock;
        servercore::Epoller _epoller;
        int _signal_fd;
        int _timer_fd;
        bool _running;

        //io单元
        servercore::ConnectionStore _connections;

        //逻辑单元
        servercore::RequestQueue<servercore::LogicRequest> _logic_requests;
        servercore::RequestQueue<servercore::LogicResult> _logic_results;
        servercore::LogicProcessor _logic;//线程池被封装在了逻辑层

        serverlogic::PageService _page_service;

        //应用层
        http::HttpHandler _http_handler;

        static const int WRITE_RETRY_INTERVAL_MS = 1000;
        static const int MAX_WRITE_RETRY = 3;
    };
}
