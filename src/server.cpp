#include "server.h"

#include <cstring>

#include "utils.h"

using namespace std;

DnsForwarder::Server::Server(const sockaddr_in &local_addr4, const sockaddr_in6 &local_addr6)
    : m_udp_server4(local_addr4), m_udp_server6(local_addr6), m_tcp_listener4(local_addr4), m_tcp_listener6(local_addr6)
{
    m_epollfd = Wrapper::EpollCreate();
    Wrapper::EpollAddFd(m_epollfd, m_udp_server4.fd(), &m_udp_server4, EPOLLIN | EPOLLET);
    Wrapper::EpollAddFd(m_epollfd, m_udp_server6.fd(), &m_udp_server6, EPOLLIN | EPOLLET);
    Wrapper::EpollAddFd(m_epollfd, m_udp_client4.fd(), &m_udp_client4, EPOLLIN | EPOLLET);
    Wrapper::EpollAddFd(m_epollfd, m_udp_client6.fd(), &m_udp_client6, EPOLLIN | EPOLLET);
    Wrapper::EpollAddFd(m_epollfd, m_tcp_listener4.fd(), &m_tcp_listener4, EPOLLIN | EPOLLET);
    Wrapper::EpollAddFd(m_epollfd, m_tcp_listener6.fd(), &m_tcp_listener6, EPOLLIN | EPOLLET);
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
    Wrapper::EpollAddFd(m_epollfd, tcp_client->fd(), tcp_client, EPOLLIN | EPOLLET | EPOLLRDHUP);
}

void DnsForwarder::Server::AddRemote(const sockaddr_in6 &remote_addr6)
{
    m_remote_addr6.push_back(remote_addr6);
    auto tcp_client = new TcpClient6(remote_addr6);
    m_tcp_client6.insert(tcp_client);
    Wrapper::EpollAddFd(m_epollfd, tcp_client->fd(), tcp_client, EPOLLIN | EPOLLET | EPOLLRDHUP);
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
                    // UdpServerRecvEvent<sockaddr_in> udp_server4_recv_event(m_udp_server4, m_udp_client4,
                    // m_udp_client6,
                    //                                                        m_remote_addr4, m_remote_addr6, m_epollfd,
                    //                                                        m_udp_task_pool);
                    // udp_server4_recv_event.Handler();
                    m_thread_pool.enqueue(UdpServerRecvHandler<sockaddr_in>, ref(m_udp_server4), ref(m_udp_client4),
                                          ref(m_udp_client6), ref(m_remote_addr4), ref(m_remote_addr6), m_epollfd,
                                          ref(m_udp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_server4.SendTo())
                        Wrapper::EpollModFd(m_epollfd, m_udp_server4.fd(), &m_udp_server4, EPOLLIN | EPOLLET);
                }
            }
            else if (socket == &m_udp_server6)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    // UdpServerRecvEvent<sockaddr_in6> udp_server6_recv_event(m_udp_server6, m_udp_client4,
                    // m_udp_client6,
                    //                                                         m_remote_addr4, m_remote_addr6,
                    //                                                         m_epollfd, m_udp_task_pool);
                    // udp_server6_recv_event.Handler();
                    m_thread_pool.enqueue(UdpServerRecvHandler<sockaddr_in6>, ref(m_udp_server6), ref(m_udp_client4),
                                          ref(m_udp_client6), ref(m_remote_addr4), ref(m_remote_addr6), m_epollfd,
                                          ref(m_udp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_server6.SendTo())
                        Wrapper::EpollModFd(m_epollfd, m_udp_server6.fd(), &m_udp_server6, EPOLLIN | EPOLLET);
                }
            }
            else if (socket == &m_udp_client4)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    // UdpClientRecvEvent<sockaddr_in> udp_client4_recv_event(m_udp_client4, m_udp_server4,
                    // m_udp_server6,
                    //                                                        m_epollfd, m_udp_task_pool);
                    // udp_client4_recv_event.Handler();
                    m_thread_pool.enqueue(UdpClientRecvHandler<sockaddr_in>, ref(m_udp_client4), ref(m_udp_server4),
                                          ref(m_udp_server6), m_epollfd, ref(m_udp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_client4.SendTo())
                        Wrapper::EpollModFd(m_epollfd, m_udp_client4.fd(), &m_udp_client4, EPOLLIN | EPOLLET);
                }
            }
            else if (socket == &m_udp_client6)
            {
                if (m_events[i].events & EPOLLIN)
                {
                    // UdpClientRecvEvent<sockaddr_in6> udp_client6_recv_event(m_udp_client6, m_udp_server4,
                    // m_udp_server6,
                    //                                                         m_epollfd, m_udp_task_pool);
                    // udp_client6_recv_event.Handler();
                    m_thread_pool.enqueue(UdpClientRecvHandler<sockaddr_in6>, ref(m_udp_client6), ref(m_udp_server4),
                                          ref(m_udp_server6), m_epollfd, ref(m_udp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!m_udp_client6.SendTo())
                        Wrapper::EpollModFd(m_epollfd, m_udp_client6.fd(), &m_udp_client6, EPOLLIN | EPOLLET);
                }
            }
            else if (socket == &m_tcp_listener4)
            {
                // TcpServerAcceptEvent<sockaddr_in> tcp_server4_accept_event(m_tcp_listener4, m_tcp_server4,
                //                                                            m_tcp_server4_mutex, m_epollfd);
                // tcp_server4_accept_event.Handler();
                m_thread_pool.enqueue(TcpServerAcceptHandler<sockaddr_in>, ref(m_tcp_listener4), ref(m_tcp_server4),
                                      ref(m_tcp_server4_mutex), m_epollfd);
            }
            else if (socket == &m_tcp_listener6)
            {
                // TcpServerAcceptEvent<sockaddr_in6> tcp_server6_accept_event(m_tcp_listener6, m_tcp_server6,
                //                                                             m_tcp_server6_mutex, m_epollfd);
                // tcp_server6_accept_event.Handler();
                m_thread_pool.enqueue(TcpServerAcceptHandler<sockaddr_in6>, ref(m_tcp_listener6), ref(m_tcp_server6),
                                      ref(m_tcp_server6_mutex), m_epollfd);
            }
            else if (m_tcp_server4.find(reinterpret_cast<TcpServer4 *>(socket)) != m_tcp_server4.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    // TcpServerRecvEvent<sockaddr_in> tcp_server4_recv_event(
                    //     reinterpret_cast<TcpServer4 *>(socket), m_tcp_client4, m_tcp_client6, m_tcp_client4_mutex,
                    //     m_tcp_client6_mutex, m_epollfd, m_tcp_task_pool);
                    // tcp_server4_recv_event.Handler();
                    m_thread_pool.enqueue(TcpServerRecvHandler<sockaddr_in>, reinterpret_cast<TcpServer4 *>(socket),
                                          ref(m_tcp_client4), ref(m_tcp_client6), ref(m_tcp_client4_mutex),
                                          ref(m_tcp_client6_mutex), ref(m_epollfd), ref(m_tcp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpServer4 *>(socket)->Send())
                        Wrapper::EpollModFd(m_epollfd, reinterpret_cast<TcpServer4 *>(socket)->fd(), socket,
                                            EPOLLIN | EPOLLET | EPOLLRDHUP);
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    // TcpServerCloseEvent<sockaddr_in> tcp_server4_close_event(
                    //     reinterpret_cast<TcpServer4 *>(socket), m_tcp_server4, m_tcp_server4_mutex, m_epollfd);
                    // tcp_server4_close_event.Handler();
                    m_thread_pool.enqueue(TcpServerCloseHandler<sockaddr_in>, reinterpret_cast<TcpServer4 *>(socket),
                                          ref(m_tcp_server4), ref(m_tcp_server4_mutex), ref(m_epollfd));
                }
            }
            else if (m_tcp_server6.find(reinterpret_cast<TcpServer6 *>(socket)) != m_tcp_server6.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    // TcpServerRecvEvent<sockaddr_in6> tcp_server6_recv_event(
                    //     reinterpret_cast<TcpServer6 *>(socket), m_tcp_client4, m_tcp_client6, m_tcp_client4_mutex,
                    //     m_tcp_client6_mutex, m_epollfd, m_tcp_task_pool);
                    // tcp_server6_recv_event.Handler();
                    m_thread_pool.enqueue(TcpServerRecvHandler<sockaddr_in6>, reinterpret_cast<TcpServer6 *>(socket),
                                          ref(m_tcp_client4), ref(m_tcp_client6), ref(m_tcp_client4_mutex),
                                          ref(m_tcp_client6_mutex), ref(m_epollfd), ref(m_tcp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpServer6 *>(socket)->Send())
                        Wrapper::EpollModFd(m_epollfd, reinterpret_cast<TcpServer6 *>(socket)->fd(), socket,
                                            EPOLLIN | EPOLLET | EPOLLRDHUP);
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    // TcpServerCloseEvent<sockaddr_in6> tcp_server6_close_event(
                    //     reinterpret_cast<TcpServer6 *>(socket), m_tcp_server6, m_tcp_server6_mutex, m_epollfd);
                    // tcp_server6_close_event.Handler();
                    m_thread_pool.enqueue(TcpServerCloseHandler<sockaddr_in6>, reinterpret_cast<TcpServer6 *>(socket),
                                          ref(m_tcp_server6), ref(m_tcp_server6_mutex), ref(m_epollfd));
                }
            }
            else if (m_tcp_client4.find(reinterpret_cast<TcpClient4 *>(socket)) != m_tcp_client4.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    // TcpClientRecvEvent<sockaddr_in> tcp_client4_recv_event(
                    //     reinterpret_cast<TcpClient4 *>(socket), m_tcp_server4, m_tcp_server6, m_tcp_server4_mutex,
                    //     m_tcp_server6_mutex, m_epollfd, m_tcp_task_pool);
                    // tcp_client4_recv_event.Handler();
                    m_thread_pool.enqueue(TcpClientRecvHandler<sockaddr_in>, reinterpret_cast<TcpClient4 *>(socket),
                                          ref(m_tcp_server4), ref(m_tcp_server6), ref(m_tcp_server4_mutex),
                                          ref(m_tcp_server6_mutex), ref(m_epollfd), ref(m_tcp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpClient4 *>(socket)->Send())
                        Wrapper::EpollModFd(m_epollfd, reinterpret_cast<TcpClient4 *>(socket)->fd(), socket,
                                            EPOLLIN | EPOLLET | EPOLLRDHUP);
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    // TcpClientCloseEvent<sockaddr_in> tcp_client4_close_event(
                    //     reinterpret_cast<TcpClient4 *>(socket), m_tcp_client4, m_tcp_client4_mutex, m_epollfd);
                    // tcp_client4_close_event.Handler();
                    m_thread_pool.enqueue(TcpClientCloseHandler<sockaddr_in>, reinterpret_cast<TcpClient4 *>(socket),
                                          ref(m_tcp_client4), ref(m_tcp_client4_mutex), ref(m_epollfd));
                }
            }
            else if (m_tcp_client6.find(reinterpret_cast<TcpClient6 *>(socket)) != m_tcp_client6.end())
            {
                if (m_events[i].events & EPOLLIN)
                {
                    // TcpClientRecvEvent<sockaddr_in6> tcp_client6_recv_event(
                    //     reinterpret_cast<TcpClient6 *>(socket), m_tcp_server4, m_tcp_server6, m_tcp_server4_mutex,
                    //     m_tcp_server6_mutex, m_epollfd, m_tcp_task_pool);
                    // tcp_client6_recv_event.Handler();
                    m_thread_pool.enqueue(TcpClientRecvHandler<sockaddr_in6>, reinterpret_cast<TcpClient6 *>(socket),
                                          ref(m_tcp_server4), ref(m_tcp_server6), ref(m_tcp_server4_mutex),
                                          ref(m_tcp_server6_mutex), ref(m_epollfd), ref(m_tcp_task_pool));
                }
                else if (m_events[i].events & EPOLLOUT)
                {
                    if (!reinterpret_cast<TcpClient6 *>(socket)->Send())
                        Wrapper::EpollModFd(m_epollfd, reinterpret_cast<TcpClient6 *>(socket)->fd(), socket,
                                            EPOLLIN | EPOLLET | EPOLLRDHUP);
                }
                else if (m_events[i].events & EPOLLRDHUP)
                {
                    // TcpClientCloseEvent<sockaddr_in6> tcp_client6_close_event(
                    //     reinterpret_cast<TcpClient6 *>(socket), m_tcp_client6, m_tcp_client6_mutex, m_epollfd);
                    // tcp_client6_close_event.Handler();
                    m_thread_pool.enqueue(TcpClientCloseHandler<sockaddr_in6>, reinterpret_cast<TcpClient6 *>(socket),
                                          ref(m_tcp_client6), ref(m_tcp_client6_mutex), ref(m_epollfd));
                }
            }
        }
    }
}
