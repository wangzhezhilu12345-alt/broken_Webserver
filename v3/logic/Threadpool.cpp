#include "Threadpool.h"
#include <unistd.h>
#include <stdio.h>  
#include <stdexcept>

Threadpool::Threadpool(int thread_num, int task_num)
    : _cond(_mutex),
      _thread_num(thread_num),
      _task_num(task_num),
      _is_exit(end_no),
      _is_joined(end_no)
{
    // 初始化线程池
    if (_thread_num <= 0 || _task_num <= 0) {
        throw std::invalid_argument("Thread number and task number must be positive");
    }

    for (int i = 0; i < _thread_num; i++) {
        pthread_t tid;
        if (pthread_create(&tid, nullptr, worker, this) != 0) {
            End(true);
            throw std::runtime_error("Failed to create thread");
        }
        _threads.push_back(tid);
    }
}

Threadpool::~Threadpool() {
    End(true);
}

bool Threadpool::PushTask(Task task) {
    MutexlockGuard lock(_mutex);
    if (_is_exit == end_ok) {
        return false;
    }



    if (static_cast<int>(_tasks.size()) >= _task_num) {
        return false;
    }

    _tasks.push(task);
    _cond.notify();
    return true;
}

//线程启动第二步
void* Threadpool::worker(void* arg) {
    Threadpool* pool = static_cast<Threadpool*>(arg);
    while (true) {
        Task task;
        if (!pool->TakeTask(task)) {
            break;
        }

        if (task._process) {
            task._process(task._arg);
        }
    }
    return nullptr;
}

void Threadpool::End(bool ok) {
    bool need_join = false;
    {
        MutexlockGuard lock(_mutex);
        _is_exit = ok ? end_ok : end_no;
        if (ok) {
            _cond.notifyAll();
            if (_is_joined == end_no) {
                _is_joined = end_ok;
                need_join = true;
            }
        }
    }

    if (need_join) {
        for (size_t i = 0; i < _threads.size(); ++i) {
            pthread_join(_threads[i], nullptr);
        }
    }
}

bool Threadpool::TakeTask(Task& task) {
    MutexlockGuard lock(_mutex);
    while (_tasks.empty() && _is_exit == end_no) {
        _cond.wait();
    }

    if (_tasks.empty() && _is_exit == end_ok) {
        return false;
    }

    task = _tasks.front();
    _tasks.pop();
    return true;
}
