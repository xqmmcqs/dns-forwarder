#ifndef _EVENT_H_
#define _EVENT_H_

#include <unordered_set>

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
                       const int &epollfd, TaskPool<UdpTask> &task_pool)
        : m_udp_server(udp_server), m_udp_client4(udp_client4), m_udp_client6(udp_client6),
          m_remote_addr4(remote_addr4), m_remote_addr6(remote_addr6), m_epollfd(epollfd), m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    UdpServer<AddrT> &m_udp_server;
    UdpClient4 &m_udp_client4;
    UdpClient6 &m_udp_client6;
    std::vector<sockaddr_in> &m_remote_addr4;
    std::vector<sockaddr_in6> &m_remote_addr6;
    const int &m_epollfd;
    TaskPool<UdpTask> &m_task_pool;
};

template <typename AddrT> class UdpClientRecvEvent : public Event
{
    UdpClientRecvEvent() = delete;
    UdpClientRecvEvent(const UdpClientRecvEvent &) = delete;
    UdpClientRecvEvent &operator=(const UdpClientRecvEvent &) = delete;

  public:
    UdpClientRecvEvent(UdpClient<AddrT> &udp_client, UdpServer4 &udp_server4, UdpServer6 &udp_server6,
                       const int &epollfd, TaskPool<UdpTask> &task_pool)
        : m_udp_client(udp_client), m_udp_server4(udp_server4), m_udp_server6(udp_server6), m_epollfd(epollfd),
          m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    UdpClient<AddrT> &m_udp_client;
    UdpServer4 &m_udp_server4;
    UdpServer6 &m_udp_server6;
    const int &m_epollfd;
    TaskPool<UdpTask> &m_task_pool;
};

template <typename AddrT> class TcpServerAcceptEvent : public Event
{
    TcpServerAcceptEvent() = delete;
    TcpServerAcceptEvent(const TcpServerAcceptEvent &) = delete;
    TcpServerAcceptEvent &operator=(const TcpServerAcceptEvent &) = delete;

  public:
    TcpServerAcceptEvent(TcpListener<AddrT> &tcp_listener, std::unordered_set<TcpServer<AddrT> *> &tcp_server,
                         const int &epollfd)
        : m_tcp_listener(tcp_listener), m_tcp_server(tcp_server), m_epollfd(epollfd)
    {
    }
    void Handler() override;

  private:
    TcpListener<AddrT> &m_tcp_listener;
    std::unordered_set<TcpServer<AddrT> *> &m_tcp_server;
    const int &m_epollfd;
};

template <typename AddrT> class TcpServerRecvEvent : public Event
{
    TcpServerRecvEvent() = delete;
    TcpServerRecvEvent(const TcpServerRecvEvent &) = delete;
    TcpServerRecvEvent &operator=(const TcpServerRecvEvent &) = delete;

  public:
    TcpServerRecvEvent(TcpServer<AddrT> *tcp_server, std::unordered_set<TcpClient4 *> &tcp_client4,
                       std::unordered_set<TcpClient6 *> &tcp_client6, const int &epollfd, TaskPool<TcpTask> &task_pool)
        : m_tcp_server(tcp_server), m_tcp_client4(tcp_client4), m_tcp_client6(tcp_client6), m_epollfd(epollfd),
          m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    TcpServer<AddrT> *m_tcp_server;
    std::unordered_set<TcpClient4 *> &m_tcp_client4;
    std::unordered_set<TcpClient6 *> &m_tcp_client6;
    const int &m_epollfd;
    TaskPool<TcpTask> &m_task_pool;
};

template <typename AddrT> class TcpClientRecvEvent : public Event
{
    TcpClientRecvEvent() = delete;
    TcpClientRecvEvent(const TcpClientRecvEvent &) = delete;
    TcpClientRecvEvent &operator=(const TcpClientRecvEvent &) = delete;

  public:
    TcpClientRecvEvent(TcpClient<AddrT> *tcp_client, std::unordered_set<TcpServer4 *> &tcp_server4,
                       std::unordered_set<TcpServer6 *> &tcp_server6, const int &epollfd, TaskPool<TcpTask> &task_pool)
        : m_tcp_client(tcp_client), m_tcp_server4(tcp_server4), m_tcp_server6(tcp_server6), m_epollfd(epollfd),
          m_task_pool(task_pool)
    {
    }
    void Handler() override;

  private:
    TcpClient<AddrT> *m_tcp_client;
    std::unordered_set<TcpServer4 *> &m_tcp_server4;
    std::unordered_set<TcpServer6 *> &m_tcp_server6;
    const int &m_epollfd;
    TaskPool<TcpTask> &m_task_pool;
};

template <typename AddrT> class TcpServerCloseEvent : public Event
{
    TcpServerCloseEvent() = delete;
    TcpServerCloseEvent(const TcpServerCloseEvent &) = delete;
    TcpServerCloseEvent &operator=(const TcpServerCloseEvent &) = delete;

  public:
    TcpServerCloseEvent(TcpServer<AddrT> *tcp_server, std::unordered_set<TcpServer<AddrT> *> &tcp_server_set,
                        const int &epollfd)
        : m_tcp_server(tcp_server), m_tcp_server_set(tcp_server_set), m_epollfd(epollfd)
    {
    }
    void Handler() override;

  private:
    TcpServer<AddrT> *m_tcp_server;
    std::unordered_set<TcpServer<AddrT> *> &m_tcp_server_set;
    const int &m_epollfd;
};

template <typename AddrT> class TcpClientCloseEvent : public Event
{
    TcpClientCloseEvent() = delete;
    TcpClientCloseEvent(const TcpClientCloseEvent &) = delete;
    TcpClientCloseEvent &operator=(const TcpClientCloseEvent &) = delete;

  public:
    TcpClientCloseEvent(TcpClient<AddrT> *tcp_client, std::unordered_set<TcpClient<AddrT> *> &tcp_client_set,
                        const int &epollfd)
        : m_tcp_client(tcp_client), m_tcp_client_set(tcp_client_set), m_epollfd(epollfd)
    {
    }
    void Handler() override;

  private:
    TcpClient<AddrT> *m_tcp_client;
    std::unordered_set<TcpClient<AddrT> *> &m_tcp_client_set;
    const int &m_epollfd;
};
} // namespace DnsForwarder

#endif // _EVENT_H_