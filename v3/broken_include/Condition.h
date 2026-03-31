#pragma once
#include <pthread.h>
#include "nocpy.h"  
#include "Mutexlock.h"

class Condition : public nocopy
{public:
    explicit Condition(Mutexlock& mutex) : _Mutex(mutex) {pthread_cond_init(&_cond, nullptr);}
    ~Condition() {pthread_cond_destroy(&_cond);}

    void wait() {pthread_cond_wait(&_cond, _Mutex.get());}
    void notify() {pthread_cond_signal(&_cond);}
    void notifyAll() {pthread_cond_broadcast(&_cond);}
private:
    Mutexlock&_Mutex;
    pthread_cond_t _cond;
};
