#pragma once

#include <memory>
#include <shared_mutex>

#include "dnspacket.h"
#include "tcpsocket.h"
#include "timer.h"
#include "udpsocket.h"

namespace DnsForwarder
{
static constexpr size_t TIMEOUT_SECONDS = 1;

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
    mutable std::shared_mutex m_queue_mutex;
    mutable std::shared_mutex m_pool_mutex;
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
    ~UdpTask()
    {
        timer->valid = false;
    }
    DnsPacket query_packet;
    union {
        sockaddr_in addr;
        sockaddr_in6 addr6;
    };
    bool is_ipv6;
    std::shared_ptr<Timer> timer;
};

struct TcpTask
{
    TcpTask() = delete;
    TcpTask(const DnsPacket &query_packet_, TcpServer4 *tcp_server4_)
        : query_packet(query_packet_), tcp_server4(tcp_server4_), is_ipv6(false)
    {
    }
    TcpTask(const DnsPacket &query_packet_, TcpServer6 *tcp_server6_)
        : query_packet(query_packet_), tcp_server6(tcp_server6_), is_ipv6(true)
    {
    }
    ~TcpTask()
    {
        timer->valid = false;
    }
    DnsPacket query_packet;
    union {
        TcpServer4 *tcp_server4;
        TcpServer6 *tcp_server6;
    };
    bool is_ipv6;
    std::shared_ptr<Timer> timer;
};
} // namespace DnsForwarder
