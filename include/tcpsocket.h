#pragma once

#include <arpa/inet.h>
#include <cstdint>

#include <mutex>
#include <queue>
#include <string>
#include <utility>

namespace DnsForwarder
{
template <typename AddrT> class TcpBase
{
  public:
    TcpBase();
    TcpBase(int sockfd) : m_sockfd(sockfd){};
    ~TcpBase();

    int fd() const
    {
        return m_sockfd;
    }

  protected:
    int m_sockfd;
};

template <typename AddrT> class TcpSocket : public TcpBase<AddrT>
{
  public:
    TcpSocket() : TcpBase<AddrT>(){};
    TcpSocket(int sockfd) : TcpBase<AddrT>(sockfd){};
    ~TcpSocket() = default;
    bool Send();
    bool Send(const std::string &data);
    void Receive(std::string &data);

  protected:
    std::mutex m_send_mutex;
    std::mutex m_recv_mutex;
    std::vector<uint8_t> m_send_buf;
    std::vector<uint8_t> m_recv_buf;
};

template <typename AddrT> class TcpClient : public TcpSocket<AddrT>
{
  public:
    TcpClient() = delete;
    TcpClient(const AddrT &addr);
    ~TcpClient() = default;
    AddrT addr() const
    {
        return m_addr;
    }

  private:
    AddrT m_addr;
};

template <typename AddrT> class TcpServer : public TcpSocket<AddrT>
{
  public:
    TcpServer() = delete;
    TcpServer(int sockfd) : TcpSocket<AddrT>(sockfd){};
    ~TcpServer() = default;
};

template <typename AddrT> class TcpListener : public TcpBase<AddrT>
{
  public:
    TcpListener() = delete;
    TcpListener(const AddrT &addr);
    ~TcpListener() = default;
    int Accept(AddrT &addr) const;
};

typedef TcpClient<sockaddr_in> TcpClient4;
typedef TcpClient<sockaddr_in6> TcpClient6;
typedef TcpServer<sockaddr_in> TcpServer4;
typedef TcpServer<sockaddr_in6> TcpServer6;
typedef TcpListener<sockaddr_in> TcpListener4;
typedef TcpListener<sockaddr_in6> TcpListener6;
} // namespace DnsForwarder