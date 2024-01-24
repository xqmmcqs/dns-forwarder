#include "threadpool.h"

#include "utils.h"
#include <thread>

using namespace std;

DnsForwarder::ThreadPool::ThreadPool(size_t threads) : m_stop(false)
{
    for (size_t i = 0; i < threads; ++i)
    {
        m_workers.emplace_back([this, i] {
            auto logger = Logger::GetInstance();
            while (true)
            {
                function<void()> task;
                {
                    unique_lock<mutex> lock(m_queue_mutex);
                    m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                    if (m_stop && m_tasks.empty())
                        return;
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }
                try
                {
                    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Thread " + to_string(i) + " is running");
                    task();
                }
                catch (const exception &e)
                {
                    auto logger = Logger::GetInstance();
                    logger.Log(__FILE__, __LINE__, Logger::ERROR, e.what());
                }
            }
        });
    }
}

DnsForwarder::ThreadPool::~ThreadPool()
{
    {
        unique_lock<mutex> lock(m_queue_mutex);
        m_stop = true;
    }
    m_condition.notify_all();
    for (auto &worker : m_workers)
        worker.join();
}