#ifndef _TCPSOCKET_H_
#define _TCPSOCKET_H_

#include <arpa/inet.h>
#include <cstdint>

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
    std::vector<char> m_send_buf;
    std::vector<char> m_recv_buf;
    std::vector<int> m_recv_len;
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
    int Accept(AddrT addr);
};

typedef TcpClient<sockaddr_in> TcpClient4;
typedef TcpClient<sockaddr_in6> TcpClient6;
typedef TcpServer<sockaddr_in> TcpServer4;
typedef TcpServer<sockaddr_in6> TcpServer6;
typedef TcpListener<sockaddr_in> TcpListener4;
typedef TcpListener<sockaddr_in6> TcpListener6;
} // namespace DnsForwarder

#endif // _TCPSOCKET_H_