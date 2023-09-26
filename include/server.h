#ifndef _SERVER_H_
#define _SERVER_H_

#include "event.h"
#include <iostream>
#include <sys/epoll.h>

namespace DnsForwarder
{
class Server
{
    Server() = delete;
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

  public:
    Server(const sockaddr_in &local_addr4, const sockaddr_in6 &local_addr6, const sockaddr_in &remote_addr4,
           const sockaddr_in6 &remote_addr6);
    void Run();

  private:
    constexpr static int MAX_EVENTS = 1024;
    const sockaddr_in m_remote_addr4;
    const sockaddr_in6 m_remote_addr6;
    UdpServer4 m_udp_server4;
    UdpClient4 m_udp_client4;
    TaskPool<UdpTask<sockaddr_in>> m_udp_task_pool4;
    UdpServerRecvEvent<sockaddr_in, sockaddr_in> m_udp_server44_recv_event;
    UdpClientRecvEvent<sockaddr_in, sockaddr_in> m_udp_client44_recv_event;
    int m_epollfd;
    epoll_event m_events[MAX_EVENTS];
};
} // namespace DnsForwarder

#endif