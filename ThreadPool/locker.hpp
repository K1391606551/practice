// 线程同步包装类
#pragma once 
#include <iostream>
#include <pthread.h>
#include <exception>
#include <semaphore.h>

// 封装信号量的类
class Sem
{
public:
    // 创建并初始化信号量
    Sem()
    {   
        if(sem_init(&_m_sem, 0, 0) != 0)
        {
            // 构造函数没有返回值，可以通过抛异常来报告错误
            throw std::exception();
        }
    }
    
    // 销毁信号量
    ~Sem()
    {
        sem_destroy(&_m_sem);
    }

    // 等待信号量
    bool wait_sem()
    {
        return sem_wait(&_m_sem) == 0;
    }

    // 增加信号量
    bool post_sem()
    {
        return sem_post(&_m_sem) == 0;
    }
private:
    sem_t _m_sem; // 声明信号量
};

// 封装互斥锁的类
class Locker
{
public:
    // 创建并初始化互斥锁
    Locker()
    {
        if(pthread_mutex_init(&_m_mutex, nullptr) != 0)
        {
            throw std::exception();
        }
    }

    // 销毁锁
    ~Locker()
    {
        pthread_mutex_destroy(&_m_mutex);
    }

    // 加锁
    bool lock()
    {
        return pthread_mutex_lock(&_m_mutex) == 0;
    }

    // 解锁
    bool unlock()
    {
        return pthread_mutex_unlock(&_m_mutex) == 0;
    }

private:
    pthread_mutex_t _m_mutex;
};

// 基于RALL思想的智能锁
class Guard_lock
{
public:
    // 资源获得及初始化
    Guard_lock(Locker *locker)
        :_locker(locker)
    {
        _locker->lock();
    }

    // 析构释放资源
    ~Guard_lock()
    {
        _locker->unlock();
        _locker->~Locker();
    }
private:
    Locker* _locker;
};

// 封装条件变量
class Cond
{
public:
    // 创建并初始化条件变量
    Cond()
    {
        if(pthread_mutex_init(&_m_mutex, nullptr) != 0)
        {
            throw std::exception();
        }

        if(pthread_cond_init(&_m_cond, nullptr) != 0)
        {
            // 构造函数一旦出现了问题就要立马释放资源了
            pthread_mutex_destroy(&_m_mutex);
            throw std::exception();
        }
    }

    // 销毁条件变量
    ~Cond()
    {
        pthread_mutex_destroy(&_m_mutex);
        pthread_cond_destroy(&_m_cond);
    }

    // 等待条件变量
    bool wait_cond()
    {
        int ret = 0;
        pthread_mutex_lock(&_m_mutex);
        // 线程在进入条件等待的时候会自动释放锁，条件满足时会优先自动的获取锁去临界区内访问临界资源
        ret = pthread_cond_wait(&_m_cond, &_m_mutex);
        
        // 感觉有点问题，不应该在访问完临界资源后解锁吗？
        pthread_mutex_unlock(&_m_mutex); 

        return ret == 0; 
    }

    // 唤醒等待条件变量线程
    bool wake_cond()
    {
        return pthread_cond_signal(&_m_cond) == 0;
    }
private:
    pthread_mutex_t _m_mutex;
    pthread_cond_t _m_cond;
};


