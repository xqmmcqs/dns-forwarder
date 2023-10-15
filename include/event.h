#ifndef _EVENT_H_
#define _EVENT_H_

#include "task.h"
#include "utils.h"

namespace DnsForwarder
{
class Event
{
  protected:
    virtual ~Event() = default;

  public:
    virtual void Handler() = 0;
};

template <typename AddrT> class UdpServerRecvEvent : public Event
{
    UdpServerRecvEvent() = delete;
    UdpServerRecvEvent(const UdpServerRecvEvent &) = delete;
    UdpServerRecvEvent &operator=(const UdpServerRecvEvent &) = delete;

  public:
    UdpServerRecvEvent(UdpServer<AddrT> &udp_server, UdpClient4 &udp_client4, UdpClient6 &udp_client6,
                       std::vector<sockaddr_in> &remote_addr4, std::vector<sockaddr_in6> &remote_addr6,
                       TaskPool<UdpTask> &task_pool)
        : m_udp_server(udp_server), m_udp_client4(udp_client4), m_udp_client6(udp_client6),
          m_remote_addr4(remote_addr4), m_remote_addr6(remote_addr6), m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    UdpServer<AddrT> &m_udp_server;
    UdpClient4 &m_udp_client4;
    UdpClient6 &m_udp_client6;
    std::vector<sockaddr_in> &m_remote_addr4;
    std::vector<sockaddr_in6> &m_remote_addr6;
    int m_epollfd;
    TaskPool<UdpTask> &m_task_pool;
};

template <typename AddrT> class UdpClientRecvEvent : public Event
{
    UdpClientRecvEvent() = delete;
    UdpClientRecvEvent(const UdpClientRecvEvent &) = delete;
    UdpClientRecvEvent &operator=(const UdpClientRecvEvent &) = delete;

  public:
    UdpClientRecvEvent(UdpClient<AddrT> &udp_client, UdpServer4 &udp_server4, UdpServer6 &udp_server6,
                       TaskPool<UdpTask> &task_pool)
        : m_udp_client(udp_client), m_udp_server4(udp_server4), m_udp_server6(udp_server6), m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    UdpClient<AddrT> &m_udp_client;
    UdpServer4 &m_udp_server4;
    UdpServer6 &m_udp_server6;
    int m_epollfd;
    TaskPool<UdpTask> &m_task_pool;
};
} // namespace DnsForwarder

#endif // _EVENT_H_