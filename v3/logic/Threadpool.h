#pragma once
#include "../broken_include/Condition.h"
#include "../broken_include/Mutexlock.h"
#include "../broken_include/nocpy.h"
#include <vector>
#include <functional>
#include <queue>
#define MAX_TASK_NUM 100
#define MAX_THREAD_NUM 10
enum end {end_ok=1,end_no=0};

struct Task
{
    Task() : _process(), _arg(nullptr) {}
    Task(std::function<void(void*)> process, void* arg)
        : _process(process), _arg(arg) {}

    std::function<void(void*)> _process;
    void* _arg;
};
class Threadpool : public nocopy
{
public:
    Threadpool(int _thread_num = MAX_THREAD_NUM, int _task_num = MAX_TASK_NUM);
    ~Threadpool();
    bool PushTask(Task task);
    static void* worker(void* arg);//pthreadcreate的回调函数
    void End(bool ok);


private:
    bool TakeTask(Task& task);

    //线程同步和互斥部分
    Mutexlock _mutex;
    Condition _cond;

    //线程池的属性
    int _thread_num;
    int _task_num;
    int _is_exit;
    int _is_joined;

    std::vector<pthread_t> _threads;   
    std::queue<Task> _tasks;
};
