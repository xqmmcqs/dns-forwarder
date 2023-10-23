#include "server.h"

#include <cstring>

#include "utils.h"

using namespace std;

DnsForwarder::Server::Server(const sockaddr_in &local_addr4, const sockaddr_in6 &local_addr6)
    : m_udp_server4(local_addr4), m_udp_server6(local_addr6), m_tcp_listener4(local_addr4), m_tcp_listener6(local_addr6)
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
    event.data.ptr = &m_tcp_listener4;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_tcp_listener4.fd(), event);
    event.data.ptr = &m_tcp_listener6;
    event.events = EPOLLIN | EPOLLET;
    Wrapper::EpollAddFd(m_epollfd, m_tcp_listener6.fd(), event);
}

DnsForwarder::Server::~Server()
{
    Wrapper::EpollDelFd(m_epollfd, m_udp_server4.fd());
    Wrapper::EpollDelFd(m_epollfd, m_udp_server6.fd());
    Wrapper::EpollDelFd(m_epollfd, m_udp_client4.fd());
    Wrapper::EpollDelFd(m_epollfd, m_udp_client6.fd());
    Wrapper::EpollDelFd(m_epollfd, m_tcp_listener4.fd());
    Wrapper::EpollDelFd(m_epollfd, m_tcp_listener6.fd());
    for (const auto &tcp_server : m_tcp_server4)
    {
        Wrapper::EpollDelFd(m_epollfd, tcp_server->fd());
        delete tcp_server;
    }
    for (const auto &tcp_server : m_tcp_server6)
    {
        Wrapper::EpollDelFd(m_epollfd, tcp_server->fd());
        delete tcp_server;
    }
    for (const auto &tcp_client : m_tcp_client4)
    {
        Wrapper::EpollDelFd(m_epollfd, tcp_client->fd());
        delete tcp_client;
    }
    for (const auto &tcp_client : m_tcp_client6)
    {
        Wrapper::EpollDelFd(m_epollfd, tcp_client->fd());
        delete tcp_client;
    }
    Wrapper::Close(m_epollfd);
}

void DnsForwarder::Server::AddRemote(const sockaddr_in &remote_addr4)
{
    m_remote_addr4.push_back(remote_addr4);
    auto tcp_client = new TcpClient4(remote_addr4);
    m_tcp_client4.insert(tcp_client);
    epoll_event event;
    event.data.ptr = tcp_client;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    Wrapper::EpollAddFd(m_epollfd, tcp_client->fd(), event);
}

void DnsForwarder::Server::AddRemote(const sockaddr_in6 &remote_addr6)
{
    m_remote_addr6.push_back(remote_addr6);
    auto tcp_client = new TcpClient6(remote_addr6);
    m_tcp_client6.insert(tcp_client);
    epoll_event event;
    event.data.ptr = tcp_client;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    Wrapper::EpollAddFd(m_epollfd, tcp_client->fd(), event);
}

void DnsForwarder::Server::Run()
{
    while (true)
    {
        int ret = Wrapper::EpollWait(m_epollfd, m_events, MAX_EVENTS, -1);
        for (int i = 0; i < ret; ++i)
        {
            void *socket = m_events[i].data.ptr;
            if (socket == &m_udp_server4)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    UdpServerRecvEvent<sockaddr_in> udp_server4_recv_event(m_udp_server4, m_udp_client4, m_udp_client6,
                                                                           m_remote_addr4, m_remote_addr6, m_epollfd,
                                                                           m_udp_task_pool);
                    udp_server4_recv_event.Handler();
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
            else if (socket == &m_udp_server6)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    UdpServerRecvEvent<sockaddr_in6> udp_server6_recv_event(m_udp_server6, m_udp_client4, m_udp_client6,
                                                                            m_remote_addr4, m_remote_addr6, m_epollfd,
                                                                            m_udp_task_pool);
                    udp_server6_recv_event.Handler();
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
            else if (socket == &m_udp_client4)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    UdpClientRecvEvent<sockaddr_in> udp_client4_recv_event(m_udp_client4, m_udp_server4, m_udp_server6,
                                                                           m_epollfd, m_udp_task_pool);
                    udp_client4_recv_event.Handler();
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
            else if (socket == &m_udp_client6)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    UdpClientRecvEvent<sockaddr_in6> udp_client6_recv_event(m_udp_client6, m_udp_server4, m_udp_server6,
                                                                            m_epollfd, m_udp_task_pool);
                    udp_client6_recv_event.Handler();
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
            else if (socket == &m_tcp_listener4)
            {
                TcpServerAcceptEvent<sockaddr_in> tcp_server4_accept_event(m_tcp_listener4, m_tcp_server4, m_epollfd);
                tcp_server4_accept_event.Handler();
            }
            else if (socket == &m_tcp_listener6)
            {
                TcpServerAcceptEvent<sockaddr_in6> tcp_server6_accept_event(m_tcp_listener6, m_tcp_server6, m_epollfd);
                tcp_server6_accept_event.Handler();
            }
            else if (m_tcp_server4.find(reinterpret_cast<TcpServer4 *>(socket)) != m_tcp_server4.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    TcpServerRecvEvent<sockaddr_in> tcp_server4_recv_event(reinterpret_cast<TcpServer4 *>(socket),
                                                                           m_tcp_client4, m_tcp_client6, m_epollfd,
                                                                           m_tcp_task_pool);
                    tcp_server4_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpServer4 *>(socket)->Send())
                    {
                        epoll_event event;
                        event.data.ptr = socket;
                        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        Wrapper::EpollModifyFd(m_epollfd, reinterpret_cast<TcpServer4 *>(socket)->fd(), event);
                    }
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    TcpServerCloseEvent<sockaddr_in> tcp_server4_close_event(reinterpret_cast<TcpServer4 *>(socket),
                                                                             m_tcp_server4, m_epollfd);
                    tcp_server4_close_event.Handler();
                }
            }
            else if (m_tcp_server6.find(reinterpret_cast<TcpServer6 *>(socket)) != m_tcp_server6.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    TcpServerRecvEvent<sockaddr_in6> tcp_server6_recv_event(reinterpret_cast<TcpServer6 *>(socket),
                                                                            m_tcp_client4, m_tcp_client6, m_epollfd,
                                                                            m_tcp_task_pool);
                    tcp_server6_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpServer6 *>(socket)->Send())
                    {
                        epoll_event event;
                        event.data.ptr = socket;
                        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        Wrapper::EpollModifyFd(m_epollfd, reinterpret_cast<TcpServer6 *>(socket)->fd(), event);
                    }
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    TcpServerCloseEvent<sockaddr_in6> tcp_server6_close_event(reinterpret_cast<TcpServer6 *>(socket),
                                                                              m_tcp_server6, m_epollfd);
                    tcp_server6_close_event.Handler();
                }
            }
            else if (m_tcp_client4.find(reinterpret_cast<TcpClient4 *>(socket)) != m_tcp_client4.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    TcpClientRecvEvent<sockaddr_in> tcp_client4_recv_event(reinterpret_cast<TcpClient4 *>(socket),
                                                                           m_tcp_server4, m_tcp_server6, m_epollfd,
                                                                           m_tcp_task_pool);
                    tcp_client4_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpClient4 *>(socket)->Send())
                    {
                        epoll_event event;
                        event.data.ptr = socket;
                        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        Wrapper::EpollModifyFd(m_epollfd, reinterpret_cast<TcpClient4 *>(socket)->fd(), event);
                    }
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    TcpClientCloseEvent<sockaddr_in> tcp_client4_close_event(reinterpret_cast<TcpClient4 *>(socket),
                                                                             m_tcp_client4, m_epollfd);
                    tcp_client4_close_event.Handler();
                }
            }
            else if (m_tcp_client6.find(reinterpret_cast<TcpClient6 *>(socket)) != m_tcp_client6.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    TcpClientRecvEvent<sockaddr_in6> tcp_client6_recv_event(reinterpret_cast<TcpClient6 *>(socket),
                                                                            m_tcp_server4, m_tcp_server6, m_epollfd,
                                                                            m_tcp_task_pool);
                    tcp_client6_recv_event.Handler();
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpClient6 *>(socket)->Send())
                    {
                        epoll_event event;
                        event.data.ptr = socket;
                        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        Wrapper::EpollModifyFd(m_epollfd, reinterpret_cast<TcpClient6 *>(socket)->fd(), event);
                    }
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    TcpClientCloseEvent<sockaddr_in6> tcp_client6_close_event(reinterpret_cast<TcpClient6 *>(socket),
                                                                              m_tcp_client6, m_epollfd);
                    tcp_client6_close_event.Handler();
                }
            }
        }
    }
}
