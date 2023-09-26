#ifndef _TASK_H_
#define _TASK_H_

#include "dnspacket.h"
#include "udpsocket.h"
#include <memory>
#include <queue>

namespace DnsForwarder
{
template <typename TaskT> class TaskPool
{
  public:
    TaskPool();
    ~TaskPool() = default;
    TaskPool(const TaskPool &) = delete;
    TaskPool &operator=(const TaskPool &) = delete;

    uint16_t PutTask(std::shared_ptr<TaskT> task);
    std::shared_ptr<TaskT> GetTask(uint16_t index);
    bool IsFull() const;
    bool InPool(uint16_t index) const;

  private:
    constexpr static int m_max_index = 65535;
    std::queue<uint16_t> m_index_queue;
    std::shared_ptr<TaskT> m_pool[m_max_index + 1];
};

template <typename AddrT> struct UdpTask
{
    UdpTask() = delete;
    UdpTask(const DnsPacket &query_packet_, const AddrT &addr_) : query_packet(query_packet_), addr(addr_)
    {
    }
    UdpTask(const UdpTask &) = default;
    UdpTask &operator=(const UdpTask &) = default;
    UdpTask(UdpTask &&) = default;
    UdpTask &operator=(UdpTask &&) = default;
    DnsPacket query_packet;
    AddrT addr;
};
} // namespace DnsForwarder

#endif