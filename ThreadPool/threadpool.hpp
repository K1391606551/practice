
#pragma once
#include <iostream>
#include <pthread.h>
#include <queue>
#include <sys/prctl.h>
#include <exception>
// 线程同步包装类
#include "locker.hpp"
#include "Task.hpp"
#include "Log.hpp"
static Locker Mutex;
// 思考为什么要弄成模板
template <class T>
class Threadpool
{
public:
    static Threadpool<T> *getinstance(int thread_number = 8, int max_queue_request_number = 100)
    {

        if (_instance == nullptr)
        {
            Mutex.lock();
            if (_instance == nullptr)
            {
                _instance = new Threadpool<T>(thread_number, max_queue_request_number);
                return _instance;
            }
            Mutex.unlock();
        }

        return _instance;
    }

    ~Threadpool()
    {
        delete[] _threads_array_tid;
        _stop = true;
    }

    bool push(T *request)
    {
        _locker_queue_request.lock();
        if (_queue_request.size() >= _max_queue_request_number)
        {
            _locker_queue_request.unlock();
            return false;
        }
        _queue_request.push(request);
        _sem_queue_request.post_sem();
        _locker_queue_request.unlock();

        return true;
    }

    void run()
    {
        std::cout << "create threads start!!!" << std::endl;
        if (_thread_number <= 0 || _max_queue_request_number <= 0)
        {
            throw std::exception();
        }

        _threads_array_tid = new pthread_t[_thread_number];
        if (nullptr == _threads_array_tid)
        {
            throw std::exception();
        }

        // 创建_thread_number个线程
        for (int i = 0; i < _thread_number; ++i)
        {
            std::cout << "create the " << i << "number thread" << std::endl;
            if (pthread_create(_threads_array_tid + i, nullptr, startroutine, (void *)this) != 0)
            {
                delete[] _threads_array_tid;
                throw std::exception();
            }

            if (pthread_detach(_threads_array_tid[i]) != 0)
            {
                delete[] _threads_array_tid;
                throw std::exception();
            }
        }
    }

private:

    static void *startroutine(void *pack)
    {
        Threadpool<T> *pthis = reinterpret_cast<Threadpool<T> *>(pack);
        prctl(PR_SET_NAME, "follower");
        pthis->loop();
    }

    void loop()
    {
        while (!_stop)
        {
            T *request = getrequest();
            if (request == nullptr)
                continue;

            // 去处理需求，需求中会自带解决处理方法handle
            request->handle();
            int one, two;
            char oper;
            request->get(&one, &two, &oper);
            // 规定，所有的任务都必须有一个run方法
            Log() << "新线程完成计算任务: " << one << oper << two << "=" << request->handle() << "\n";
        }
    }

    Threadpool(int thread_number = 8, int max_queue_request_number = 100)
        : _thread_number(thread_number), _max_queue_request_number(max_queue_request_number), _stop(false), _threads_array_tid(nullptr)
    {
    }

    Threadpool(const Threadpool<T> &) = delete;
    Threadpool<T> &operator=(const Threadpool<T> &) = delete;
    
    T *getrequest()
    {
        _sem_queue_request.wait_sem();
        _locker_queue_request.lock();
        // 感觉有点多余，可以获得信号量说明一定有数据
        if (_queue_request.empty())
        {
            _locker_queue_request.unlock();
            return nullptr;
        }

        T *temp = _queue_request.front();
        _queue_request.pop();
        _locker_queue_request.unlock();

        return temp;
    }

private:
    int _thread_number;              // 线程池中的线程数量
    int _max_queue_request_number;   // 请求队列中允许请求的最带请求数
    std::queue<T *> _queue_request;  // 请求队列
    pthread_t *_threads_array_tid;   // 存放线程id的数组
    Locker _locker_queue_request;    // 保护请求队列的互斥锁
    Sem _sem_queue_request;          // 是否有任务需要处理
    bool _stop;                      // 是否结束线程
    static Threadpool<T> *_instance; // 单例对象，懒汉模式
};

template <class T>
Threadpool<T> *Threadpool<T>::_instance = nullptr;