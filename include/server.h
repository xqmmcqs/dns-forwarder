#ifndef _SERVER_H_
#define _SERVER_H_

#include <sys/epoll.h>

#include <iostream>

#include "event.h"

namespace DnsForwarder
{
class Server
{
    Server() = delete;
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

  public:
    Server(const sockaddr_in &local_addr4, const sockaddr_in6 &local_addr6);
    void AddRemote(const sockaddr_in &remote_addr4);
    void AddRemote(const sockaddr_in6 &remote_addr6);
    void Run();

  private:
    constexpr static int MAX_EVENTS = 1024;
    UdpServer4 m_udp_server4;
    UdpServer6 m_udp_server6;
    UdpClient4 m_udp_client4;
    UdpClient6 m_udp_client6;
    std::vector<sockaddr_in> m_remote_addr4;
    std::vector<sockaddr_in6> m_remote_addr6;
    TaskPool<UdpTask> m_udp_task_pool;
    int m_epollfd;
    epoll_event m_events[MAX_EVENTS];
};
} // namespace DnsForwarder

#endif // _SERVER_H_