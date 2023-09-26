#ifndef _EVENT_H_
#define _EVENT_H_

#include "task.h"

namespace DnsForwarder
{
class Event
{
  protected:
    virtual ~Event() = default;

  public:
    virtual void Handler() = 0;
};

template <typename ServerAddrT, typename ClientAddrT> class UdpServerRecvEvent : public Event
{
    UdpServerRecvEvent() = delete;
    UdpServerRecvEvent(const UdpServerRecvEvent &) = delete;
    UdpServerRecvEvent &operator=(const UdpServerRecvEvent &) = delete;

  public:
    UdpServerRecvEvent(UdpServer<ServerAddrT> &udp_server, UdpClient<ClientAddrT> &udp_client,
                       const ClientAddrT &remote_addr, TaskPool<UdpTask<ServerAddrT>> &task_pool)
        : m_udp_server(udp_server), m_udp_client(udp_client), m_remote_addr(remote_addr), m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    UdpServer<ServerAddrT> &m_udp_server;
    UdpClient<ClientAddrT> &m_udp_client;
    const ClientAddrT &m_remote_addr;
    TaskPool<UdpTask<ServerAddrT>> &m_task_pool;
};

template <typename ServerAddrT, typename ClientAddrT> class UdpClientRecvEvent : public Event
{
    UdpClientRecvEvent() = delete;
    UdpClientRecvEvent(const UdpClientRecvEvent &) = delete;
    UdpClientRecvEvent &operator=(const UdpClientRecvEvent &) = delete;

  public:
    UdpClientRecvEvent(UdpServer<ServerAddrT> &udp_server, UdpClient<ClientAddrT> &udp_client,
                       const ClientAddrT &remote_addr, TaskPool<UdpTask<ServerAddrT>> &task_pool)
        : m_udp_server(udp_server), m_udp_client(udp_client), m_remote_addr(remote_addr), m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    UdpServer<ServerAddrT> &m_udp_server;
    UdpClient<ClientAddrT> &m_udp_client;
    const ClientAddrT &m_remote_addr;
    TaskPool<UdpTask<ServerAddrT>> &m_task_pool;
};
} // namespace DnsForwarder

#endif