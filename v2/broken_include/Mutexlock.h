#pragma once
#include <pthread.h>
#include "nocpy.h"
class Mutexlock : public nocopy
{
public:
    Mutexlock() {pthread_mutex_init(&_mutex, nullptr);}
    ~Mutexlock() {pthread_mutex_destroy(&_mutex);}
    void lock() {pthread_mutex_lock(&_mutex);}
    void unlock() {pthread_mutex_unlock(&_mutex);}
    pthread_mutex_t* get() {return &_mutex;}
private:
    pthread_mutex_t _mutex;
};

//这个在关键地方上锁的自动函数
class MutexlockGuard: public nocopy
{
public: 
    MutexlockGuard(Mutexlock& mutex) : _mutex(mutex) {_mutex.lock();}
    ~MutexlockGuard() {_mutex.unlock();}
private:
    Mutexlock& _mutex;
};