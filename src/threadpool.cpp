#include "threadpool.h"

using namespace std;

DnsForwarder::ThreadPool::ThreadPool(size_t threads) : m_stop(false)
{
    for (size_t i = 0; i < threads; ++i)
        m_workers.emplace_back([this] { this->Handler(); });
}

DnsForwarder::ThreadPool::~ThreadPool()
{
    {
        unique_lock<mutex> lock(m_queue_mutex);
        m_stop = true;
    }
    m_condition.notify_all();
    for (thread &worker : m_workers)
        worker.join();
}

void DnsForwarder::ThreadPool::Handler()
{
    for (;;)
    {
        function<void()> task;
        {
            unique_lock<mutex> lock(m_queue_mutex);
            m_condition.wait(lock, [this] { return this->m_stop || !this->m_tasks.empty(); });
            if (m_stop && m_tasks.empty())
                return;
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        task();
    }
}
