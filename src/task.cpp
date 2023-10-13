#include "task.h"

using namespace std;

template <typename TaskT> DnsForwarder::TaskPool<TaskT>::TaskPool() : m_index_queue(), m_pool()
{
    for (int i = 0; i <= m_max_index; ++i)
    {
        m_index_queue.push(i);
    }
}

template <typename TaskT> uint16_t DnsForwarder::TaskPool<TaskT>::PutTask(shared_ptr<TaskT> task)
{
    uint16_t index = m_index_queue.front();
    m_index_queue.pop();
    m_pool[index] = task;
    return index;
}

template <typename TaskT> shared_ptr<TaskT> DnsForwarder::TaskPool<TaskT>::GetTask(uint16_t index) const
{
    if (!m_pool[index])
        return nullptr;
    return m_pool[index];
}

template <typename TaskT> void DnsForwarder::TaskPool<TaskT>::DelTask(uint16_t index)
{
    if (m_pool[index])
    {
        m_index_queue.push(index);
        m_pool[index] = nullptr;
    }
}

template <typename TaskT> bool DnsForwarder::TaskPool<TaskT>::IsFull() const
{
    return m_index_queue.empty();
}

template <typename TaskT> bool DnsForwarder::TaskPool<TaskT>::HasTask(uint16_t index) const
{
    return m_pool[index] != nullptr;
}

template class DnsForwarder::TaskPool<DnsForwarder::UdpTask>;