#include "task.h"

using namespace std;

template <typename TaskT> DnsForwarder::TaskPool<TaskT>::TaskPool() : m_index_queue(), m_pool()
{
    for (int i = 0; i <= m_max_index; ++i)
        m_index_queue.push(i);
}

template <typename TaskT> uint16_t DnsForwarder::TaskPool<TaskT>::PutTask(shared_ptr<TaskT> task)
{
    uint16_t index;
    {
        unique_lock<shared_mutex> queue_lock(m_queue_mutex);
        index = m_index_queue.front();
        m_index_queue.pop();
    }
    {
        unique_lock<shared_mutex> pool_lock(m_pool_mutex);
        m_pool[index] = task;
        task->timer = make_shared<Timer>(chrono::milliseconds(TIMEOUT_SECONDS * 1000), index);
    }
    return index;
}

template <typename TaskT> shared_ptr<TaskT> DnsForwarder::TaskPool<TaskT>::GetTask(uint16_t index) const
{
    shared_lock<shared_mutex> pool_lock(m_pool_mutex);
    return m_pool[index] ? m_pool[index] : nullptr;
}

template <typename TaskT> void DnsForwarder::TaskPool<TaskT>::DelTask(uint16_t index)
{
    unique_lock<shared_mutex> pool_lock(m_pool_mutex);
    if (m_pool[index])
    {
        m_pool[index] = nullptr;
        unique_lock<shared_mutex> queue_lock(m_queue_mutex);
        m_index_queue.push(index);
    }
}

template <typename TaskT> bool DnsForwarder::TaskPool<TaskT>::IsFull() const
{
    shared_lock<shared_mutex> queue_lock(m_queue_mutex);
    return m_index_queue.empty();
}

template <typename TaskT> bool DnsForwarder::TaskPool<TaskT>::HasTask(uint16_t index) const
{
    shared_lock<shared_mutex> pool_lock(m_pool_mutex);
    return m_pool[index] != nullptr;
}

template class DnsForwarder::TaskPool<DnsForwarder::UdpTask>;
template class DnsForwarder::TaskPool<DnsForwarder::TcpTask>;