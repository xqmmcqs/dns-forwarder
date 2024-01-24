#pragma once

#include <unordered_set>

#include "task.h"
#include "timer.h"
#include "utils.h"

namespace DnsForwarder
{
template <typename AddrT>
void UdpServerRecvHandler(const UdpServer<AddrT> &udp_server, UdpClient4 &udp_client4, UdpClient6 &udp_client6,
                          const std::vector<sockaddr_in> &remote_addr4, const std::vector<sockaddr_in6> &remote_addr6,
                          const int &epollfd, TaskPool<UdpTask> &task_pool, TimerHeap &timer_heap);

template <typename AddrT>
void UdpClientRecvHandler(const UdpClient<AddrT> &udp_client, UdpServer4 &udp_server4, UdpServer6 &udp_server6,
                          const int &epollfd, TaskPool<UdpTask> &task_pool);

template <typename AddrT>
void TcpServerAcceptHandler(const TcpListener<AddrT> &tcp_listener,
                            std::unordered_set<TcpServer<AddrT> *> &tcp_server_set, std::shared_mutex &tcp_server_mutex,
                            const int &epollfd);

template <typename AddrT>
void TcpServerRecvHandler(TcpServer<AddrT> *tcp_server, const std::unordered_set<TcpClient4 *> &tcp_client4,
                          const std::unordered_set<TcpClient6 *> &tcp_client6, std::shared_mutex &tcp_client4_mutex,
                          std::shared_mutex &tcp_client6_mutex, const int &epollfd, TaskPool<TcpTask> &task_pool,
                          TimerHeap &timer_heap);

template <typename AddrT>
void TcpClientRecvHandler(TcpClient<AddrT> *tcp_client, const std::unordered_set<TcpServer4 *> &tcp_server4,
                          const std::unordered_set<TcpServer6 *> &tcp_server6, std::shared_mutex &tcp_server4_mutex,
                          std::shared_mutex &tcp_server6_mutex, const int &epollfd, TaskPool<TcpTask> &task_pool);

template <typename AddrT>
void TcpServerCloseHandler(TcpServer<AddrT> *tcp_server, std::unordered_set<TcpServer<AddrT> *> &tcp_server_set,
                           std::shared_mutex &tcp_server_mutex, const int &epollfd);

template <typename AddrT>
void TcpClientCloseHandler(TcpClient<AddrT> *tcp_client, std::unordered_set<TcpClient<AddrT> *> &tcp_client_set,
                           std::shared_mutex &tcp_client_mutex, const int &epollfd);

void UdpTimeoutHandler(UdpServer4 &udp_server4, UdpServer6 &udp_server6, const int &epollfd,
                       TaskPool<UdpTask> &task_pool, TimerHeap &timer_heap);

void TcpTimeoutHandler(const std::unordered_set<TcpServer4 *> &tcp_server4,
                       const std::unordered_set<TcpServer6 *> &tcp_server6, std::shared_mutex &tcp_server4_mutex,
                       std::shared_mutex &tcp_server6_mutex, const int &epollfd, TaskPool<TcpTask> &task_pool,
                       TimerHeap &timer_heap);
} // namespace DnsForwarder