#include "udpsocket.h"

#include <stdexcept>

#include "utils.h"

using namespace std;

template <> DnsForwarder::UdpSocket4::UdpSocket()
{
    m_sockfd = Wrapper::Socket(AF_INET, SOCK_DGRAM, 0);
}

template <> DnsForwarder::UdpSocket6::UdpSocket()
{
    m_sockfd = Wrapper::Socket(AF_INET6, SOCK_DGRAM, 0);
}

template <typename AddrT> DnsForwarder::UdpSocket<AddrT>::UdpSocket()
{
    throw runtime_error("Invalid socket type");
}

template <typename AddrT> DnsForwarder::UdpSocket<AddrT>::~UdpSocket()
{
    Wrapper::Close(m_sockfd);
}

template <typename AddrT> bool DnsForwarder::UdpSocket<AddrT>::SendTo()
{
    while (!send_queue.empty())
    {
        const auto &packet = send_queue.front();
        if (!Wrapper::SendTo(m_sockfd, packet.second.c_str(), packet.second.size(), MSG_DONTWAIT,
                             (struct sockaddr *)&packet.first, sizeof(packet.first)))
            break;
        send_queue.pop();
    }
    return !send_queue.empty();
}

template <typename AddrT> bool DnsForwarder::UdpSocket<AddrT>::SendTo(const AddrT &addr, const std::string &data)
{
    if (!send_queue.empty())
    {
        send_queue.push(make_pair(addr, data));
        return false;
    }
    if (!Wrapper::SendTo(m_sockfd, data.c_str(), data.size(), MSG_DONTWAIT, (struct sockaddr *)&addr, sizeof(addr)))
    {
        send_queue.push(make_pair(addr, data));
        return true;
    }
    return false;
}

template <typename AddrT> void DnsForwarder::UdpSocket<AddrT>::ReceiveFrom(AddrT &addr, std::string &data) const
{
    char buf[65536];
    socklen_t addrlen(sizeof(addr));
    ssize_t nrecv = Wrapper::RecvFrom(m_sockfd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr *)&addr, &addrlen);
    if (nrecv)
        data.assign(buf, nrecv);
}

template <typename AddrT> DnsForwarder::UdpServer<AddrT>::UdpServer(const AddrT &addr)
{
    Wrapper::Bind(this->m_sockfd, (struct sockaddr *)&addr, sizeof(addr));
}

template class DnsForwarder::UdpSocket<sockaddr_in>;
template class DnsForwarder::UdpSocket<sockaddr_in6>;
template class DnsForwarder::UdpClient<sockaddr_in>;
template class DnsForwarder::UdpClient<sockaddr_in6>;
template class DnsForwarder::UdpServer<sockaddr_in>;
template class DnsForwarder::UdpServer<sockaddr_in6>;