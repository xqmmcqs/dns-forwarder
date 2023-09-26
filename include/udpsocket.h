#ifndef _UDPSOCKET_H_
#define _UDPSOCKET_H_

#include <arpa/inet.h>
#include <stdint.h>

#include <queue>
#include <string>
#include <utility>

namespace DnsForwarder
{
template <typename AddrT> class UdpSocket
{
    UdpSocket(const UdpSocket &) = delete;
    UdpSocket &operator=(const UdpSocket &) = delete;

  public:
    UdpSocket();
    ~UdpSocket();
    int fd() const
    {
        return m_sockfd;
    }
    void SendTo();
    void SendTo(const AddrT &addr, const std::string &data);
    void ReceiveFrom(AddrT &addr, std::string &data) const;

  protected:
    int m_sockfd;
    std::queue<std::pair<AddrT, std::string>> send_queue;
};

template <typename AddrT> class UdpClient : public UdpSocket<AddrT>
{
  public:
    UdpClient() = default;
    ~UdpClient() = default;
};

template <typename AddrT> class UdpServer : public UdpSocket<AddrT>
{
  public:
    UdpServer() = delete;
    UdpServer(const AddrT &addr);
    ~UdpServer() = default;

  private:
    AddrT m_addr;
};

typedef UdpSocket<sockaddr_in> UdpSocket4;
typedef UdpSocket<sockaddr_in6> UdpSocket6;
typedef UdpClient<sockaddr_in> UdpClient4;
typedef UdpClient<sockaddr_in6> UdpClient6;
typedef UdpServer<sockaddr_in> UdpServer4;
typedef UdpServer<sockaddr_in6> UdpServer6;
} // namespace DnsForwarder

#endif