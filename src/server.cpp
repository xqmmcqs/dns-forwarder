#include "server.h"
#include "utils.h"
#include <cstring>
using namespace std;

DnsForwarder::Server::Server(const sockaddr_in &local_addr4, const sockaddr_in6 &local_addr6,
                             const sockaddr_in &remote_addr4, const sockaddr_in6 &remote_addr6)
    : m_remote_addr4(remote_addr4), m_remote_addr6(remote_addr6), m_udp_server4(local_addr4), m_udp_client4(),
      m_udp_task_pool4(), m_udp_server44_recv_event(m_udp_server4, m_udp_client4, m_remote_addr4, m_udp_task_pool4),
      m_udp_client44_recv_event(m_udp_server4, m_udp_client4, m_remote_addr4, m_udp_task_pool4)
{
    m_epollfd = Wrapper::EpollCreate();
    epoll_event event;
    event.data.ptr = &m_udp_server4;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_udp_server4.fd(), event);

    event.data.ptr = &m_udp_client4;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_udp_client4.fd(), event);
}

void DnsForwarder::Server::Run()
{
    while (true)
    {
        int ret = Wrapper::EpollWait(m_epollfd, m_events, MAX_EVENTS, -1);
        for (int i = 0; i < ret; ++i)
        {
            auto *socket = m_events[i].data.ptr;
            if (socket == &m_udp_server4)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    m_udp_server44_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    m_udp_server4.SendTo();
                }
            }
            else if (socket == &m_udp_client4)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    m_udp_client44_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    m_udp_client4.SendTo();
                }
            }
        }
    }
}
