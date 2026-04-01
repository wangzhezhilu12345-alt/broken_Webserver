#pragma once
#include "../broken_include/Condition.h"
#include "../broken_include/Mutexlock.h"
#include "../broken_include/nocpy.h"
#include <queue>

namespace servercore
{
    template <typename T>
    class RequestQueue : public nocopy
    {
    public:
        RequestQueue()
            : _not_empty(_mutex),
              _closed(false)
        {
        }

        bool Push(const T &value)
        {
            MutexlockGuard lock(_mutex);
            if (_closed)
            {
                return false;
            }
            _queue.push(value);
            _not_empty.notify();
            return true;
        }

        bool Pop(T &value)
        {
            MutexlockGuard lock(_mutex);
            while (_queue.empty() && !_closed)
            {
                _not_empty.wait();
            }
            if (_queue.empty())
            {
                return false;
            }
            value = _queue.front();
            _queue.pop();
            return true;
        }

        bool TryPop(T &value)
        {
            MutexlockGuard lock(_mutex);
            if (_queue.empty())
            {
                return false;
            }
            value = _queue.front();
            _queue.pop();
            return true;
        }

        void Close()
        {
            MutexlockGuard lock(_mutex);
            _closed = true;
            _not_empty.notifyAll();
        }

    private:
        Mutexlock _mutex;
        Condition _not_empty;
        std::queue<T> _queue;
        bool _closed;
    };
}
