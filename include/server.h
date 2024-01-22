#pragma once

#include <sys/epoll.h>

#include <iostream>

#include "event.h"
#include "handler.h"
#include "threadpool.h"

namespace DnsForwarder
{
class Server
{
    Server() = delete;
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

  public:
    Server(const sockaddr_in &local_addr4, const sockaddr_in6 &local_addr6);
    ~Server();
    void AddRemote(const sockaddr_in &remote_addr4);
    void AddRemote(const sockaddr_in6 &remote_addr6);
    void Run();

  private:
    constexpr static int MAX_EVENTS = 1024;
    UdpServer4 m_udp_server4;
    UdpServer6 m_udp_server6;
    UdpClient4 m_udp_client4;
    UdpClient6 m_udp_client6;
    TcpListener4 m_tcp_listener4;
    TcpListener6 m_tcp_listener6;
    std::unordered_set<TcpServer4 *> m_tcp_server4;
    std::unordered_set<TcpServer6 *> m_tcp_server6;
    std::unordered_set<TcpClient4 *> m_tcp_client4;
    std::unordered_set<TcpClient6 *> m_tcp_client6;
    std::shared_mutex m_tcp_server4_mutex;
    std::shared_mutex m_tcp_server6_mutex;
    std::shared_mutex m_tcp_client4_mutex;
    std::shared_mutex m_tcp_client6_mutex;
    std::vector<sockaddr_in> m_remote_addr4;
    std::vector<sockaddr_in6> m_remote_addr6;
    ThreadPool m_thread_pool;
    TaskPool<UdpTask> m_udp_task_pool;
    TaskPool<TcpTask> m_tcp_task_pool;
    int m_epollfd;
    epoll_event m_events[MAX_EVENTS];
};
} // namespace DnsForwarder