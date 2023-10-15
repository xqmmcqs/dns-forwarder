#ifndef _TASK_H_
#define _TASK_H_

#include <memory>

#include "dnspacket.h"
#include "udpsocket.h"

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
    std::shared_ptr<TaskT> GetTask(uint16_t index) const;
    void DelTask(uint16_t index);
    bool IsFull() const;
    bool HasTask(uint16_t index) const;

  private:
    constexpr static int m_max_index = 65535;
    std::queue<uint16_t> m_index_queue;
    std::shared_ptr<TaskT> m_pool[m_max_index + 1];
};

struct UdpTask
{
    UdpTask() = delete;
    UdpTask(const DnsPacket &query_packet_, sockaddr_in addr_)
        : query_packet(query_packet_), addr(addr_), is_ipv6(false)
    {
    }
    UdpTask(const DnsPacket &query_packet_, sockaddr_in6 addr_)
        : query_packet(query_packet_), addr6(addr_), is_ipv6(true)
    {
    }
    DnsPacket query_packet;
    union {
        sockaddr_in addr;
        sockaddr_in6 addr6;
    };
    bool is_ipv6;
};
} // namespace DnsForwarder

#endif // _TASK_H_