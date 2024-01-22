#include "threadpool.h"

using namespace std;

DnsForwarder::ThreadPool::ThreadPool(size_t threads) : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back([this] { this->Handler(); });
}

DnsForwarder::ThreadPool::~ThreadPool()
{
    {
        unique_lock<mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (thread &worker : workers)
        worker.join();
}

void DnsForwarder::ThreadPool::Handler()
{
    for (;;)
    {
        function<void()> task;
        {
            unique_lock<mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
            if (stop && tasks.empty())
                return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}
