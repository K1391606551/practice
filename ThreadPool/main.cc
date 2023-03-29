#include "threadpool.hpp"
#include "Log.hpp"
#include "Task.hpp"
#include <memory>
#include <ctime>
#include <thread>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    prctl(PR_SET_NAME, "leader"); // 对线程名进行更改
    const std::string ops = "+-*/%";
    // std::unique_ptr<ThreadPool<Task>> tp(new ThreadPool<Task>());
    std::unique_ptr<Threadpool<Task>> tp(Threadpool<Task>::getinstance());
    tp->run();
    // 主线程派发任务
    srand((unsigned long)time(nullptr) ^ getpid());
    while(true)
    {
        int a = rand() % 20;
        int b = rand() % 10;
        char op = ops[rand() % ops.size()];
        Task t(a, b , op);
        Log() << "主线程派发计算任务: " << a << op << b << "=?" << "\n";
        // 向线程池的需求队列里面放request
        tp->push(&t);
        
        sleep(1);
    }

    return 0;
}
