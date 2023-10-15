#include "server.h"

#include <cstring>

#include "utils.h"

using namespace std;

DnsForwarder::Server::Server(const sockaddr_in &local_addr4, const sockaddr_in6 &local_addr6)
    : m_udp_server4(local_addr4), m_udp_server6(local_addr6)
{
    m_epollfd = Wrapper::EpollCreate();
    epoll_event event;
    event.data.ptr = &m_udp_server4;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_udp_server4.fd(), event);
    event.data.ptr = &m_udp_server6;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_udp_server6.fd(), event);
    event.data.ptr = &m_udp_client4;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_udp_client4.fd(), event);
    event.data.ptr = &m_udp_client6;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_udp_client6.fd(), event);
}

void DnsForwarder::Server::AddRemote(const sockaddr_in &remote_addr4)
{
    m_remote_addr4.push_back(remote_addr4);
}

void DnsForwarder::Server::AddRemote(const sockaddr_in6 &remote_addr6)
{
    m_remote_addr6.push_back(remote_addr6);
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
                    UdpServerRecvEvent<sockaddr_in> m_udp_server4_recv_event(
                        m_udp_server4, m_udp_client4, m_udp_client6, m_remote_addr4, m_remote_addr6, m_udp_task_pool);
                    m_udp_server4_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_server4.SendTo())
                    {
                        epoll_event event;
                        event.data.ptr = &m_udp_server4;
                        event.events = EPOLLIN | EPOLLET;
                        Wrapper::EpollModifyFd(m_epollfd, m_udp_server4.fd(), event);
                    }
                }
            }
            else if (socket == &m_udp_client4)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    UdpClientRecvEvent<sockaddr_in> m_udp_client4_recv_event(m_udp_client4, m_udp_server4,
                                                                             m_udp_server6, m_udp_task_pool);
                    m_udp_client4_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_client4.SendTo())
                    {
                        epoll_event event;
                        event.data.ptr = &m_udp_client4;
                        event.events = EPOLLIN | EPOLLET;
                        Wrapper::EpollModifyFd(m_epollfd, m_udp_client4.fd(), event);
                    }
                }
            }
            else if (socket == &m_udp_server6)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    UdpServerRecvEvent<sockaddr_in6> m_udp_server6_recv_event(
                        m_udp_server6, m_udp_client4, m_udp_client6, m_remote_addr4, m_remote_addr6, m_udp_task_pool);
                    m_udp_server6_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_server6.SendTo())
                    {
                        epoll_event event;
                        event.data.ptr = &m_udp_server6;
                        event.events = EPOLLIN | EPOLLET;
                        Wrapper::EpollModifyFd(m_epollfd, m_udp_server6.fd(), event);
                    }
                }
            }
            else if (socket == &m_udp_client6)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    UdpClientRecvEvent<sockaddr_in6> m_udp_client6_recv_event(m_udp_client6, m_udp_server4,
                                                                              m_udp_server6, m_udp_task_pool);
                    m_udp_client6_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_client6.SendTo())
                    {
                        epoll_event event;
                        event.data.ptr = &m_udp_client6;
                        event.events = EPOLLIN | EPOLLET;
                        Wrapper::EpollModifyFd(m_epollfd, m_udp_client6.fd(), event);
                    }
                }
            }
        }
    }
}
