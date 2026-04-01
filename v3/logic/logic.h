#pragma once
#include "../io/connection.h"
#include "page_service.h"
#include "request_queue.h"
#include "Threadpool.h"
#include "../broken_include/nocpy.h"
#include <functional>
#include <memory>
#include <string>

namespace servercore
{
    struct LogicRequest
    {
        std::shared_ptr<Connection> _connection;
        http::HttpRequest _request;
    };

    struct LogicResult
    {
        std::shared_ptr<Connection> _connection;
        http::HttpRequest _request;
        serverlogic::PageResult _page;
    };

    class LogicProcessor : public nocopy
    {
    public:
        typedef std::function<LogicResult(const LogicRequest &)> RequestHandler;

        LogicProcessor(RequestQueue<LogicRequest> &request_queue,
                       RequestQueue<LogicResult> &result_queue,
                       int thread_num = MAX_THREAD_NUM,
                       int task_num = MAX_TASK_NUM);
        ~LogicProcessor();

        void SetHandler(const RequestHandler &handler);

    private:
        static void WorkerLoop(void *arg);

    private:
        RequestQueue<LogicRequest> &_request_queue;
        RequestQueue<LogicResult> &_result_queue;
        RequestHandler _handler;
        Threadpool _threadpool;
        int _worker_num;
    };
}
