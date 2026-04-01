#include "logic.h"

servercore::LogicProcessor::LogicProcessor(RequestQueue<LogicRequest> &request_queue,
                                           RequestQueue<LogicResult> &result_queue,
                                           int thread_num,
                                           int task_num)
    : _request_queue(request_queue),
      _result_queue(result_queue),
      _handler(),
      _threadpool(thread_num, task_num),
      _worker_num(thread_num)
{
    for (int i = 0; i < _worker_num; ++i)
    {
        //这个任务函数被投递给了线程池...，这个this相当于对象上下文,设计用void*会好一点
        _threadpool.PushTask(Task(WorkerLoop, this));
    }
}

servercore::LogicProcessor::~LogicProcessor()
{
    _request_queue.Close();
}

void servercore::LogicProcessor::SetHandler(const RequestHandler &handler)
{
    _handler = handler;
}

void servercore::LogicProcessor::WorkerLoop(void *arg)
{
    LogicProcessor *processor = static_cast<LogicProcessor *>(arg);
    LogicRequest request;
    while (processor->_request_queue.Pop(request))
    {
        if (!processor->_handler)
        {
            continue;
        }

        LogicResult result = processor->_handler(request);
        processor->_result_queue.Push(result);
    }
}
