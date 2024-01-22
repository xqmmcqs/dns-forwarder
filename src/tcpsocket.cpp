#include "tcpsocket.h"

#include "utils.h"

using namespace std;

template <> DnsForwarder::TcpBase<sockaddr_in>::TcpBase()
{
    m_sockfd = Wrapper::Socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    Wrapper::SetSockOpt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

template <> DnsForwarder::TcpBase<sockaddr_in6>::TcpBase()
{
    m_sockfd = Wrapper::Socket(AF_INET6, SOCK_STREAM, 0);
    int reuse = 1;
    Wrapper::SetSockOpt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

template <typename AddrT> DnsForwarder::TcpBase<AddrT>::TcpBase()
{
    throw runtime_error("Invalid socket type");
}

template <typename AddrT> DnsForwarder::TcpBase<AddrT>::~TcpBase()
{
    Wrapper::Close(m_sockfd);
}

template <typename AddrT> bool DnsForwarder::TcpSocket<AddrT>::Send()
{
    unique_lock<mutex> lock(m_send_mutex);
    auto n = Wrapper::Send(this->m_sockfd, m_send_buf.data(), m_send_buf.size(), 0);
    m_send_buf = vector<uint8_t>(make_move_iterator(m_send_buf.begin() + n), make_move_iterator(m_send_buf.end()));
    return !m_send_buf.empty();
}

template <typename AddrT> bool DnsForwarder::TcpSocket<AddrT>::Send(const std::string &data)
{
    unique_lock<mutex> lock(m_send_mutex);
    if (!m_send_buf.empty())
    {
        m_send_buf.insert(m_send_buf.end(), data.begin(), data.end());
        return false;
    }
    auto n = Wrapper::Send(this->m_sockfd, data.c_str(), data.size(), 0);
    if (n < data.size())
    {
        m_send_buf.insert(m_send_buf.end(), data.begin() + n, data.end());
        return true;
    }
    return false;
}

template <typename AddrT> void DnsForwarder::TcpSocket<AddrT>::Receive(std::string &data)
{
    uint8_t buf[65536];
    auto nrecv = Wrapper::Recv(this->m_sockfd, buf, sizeof(buf), 0);
    unique_lock<mutex> lock(m_recv_mutex);
    if (nrecv)
        m_recv_buf.insert(m_recv_buf.end(), buf, buf + nrecv);
    if (m_recv_buf.empty())
        return;
    int n = 0;
    while (n < m_recv_buf.size() && n + m_recv_buf[n] * 256 + m_recv_buf[n + 1] + 2 <= m_recv_buf.size())
        n += m_recv_buf[n] * 256 + m_recv_buf[n + 1] + 2;
    if (n)
    {
        data = string(make_move_iterator(m_recv_buf.begin()), make_move_iterator(m_recv_buf.begin() + n));
        m_recv_buf = vector<uint8_t>(make_move_iterator(m_recv_buf.begin() + n), make_move_iterator(m_recv_buf.end()));
    }
    return;
}

template <typename AddrT> DnsForwarder::TcpClient<AddrT>::TcpClient(const AddrT &addr) : TcpSocket<AddrT>()
{
    Wrapper::Connect(this->m_sockfd, (struct sockaddr *)&addr, sizeof(addr));
}

template <typename AddrT> DnsForwarder::TcpListener<AddrT>::TcpListener(const AddrT &addr) : TcpBase<AddrT>()
{
    Wrapper::Bind(this->m_sockfd, (struct sockaddr *)&addr, sizeof(addr));
    Wrapper::Listen(this->m_sockfd, 5);
}

template <typename AddrT> int DnsForwarder::TcpListener<AddrT>::Accept(AddrT &addr) const
{
    socklen_t addrlen;
    return Wrapper::Accept(this->m_sockfd, (sockaddr *)&addr, &addrlen);
}

template class DnsForwarder::TcpBase<sockaddr_in>;
template class DnsForwarder::TcpBase<sockaddr_in6>;
template class DnsForwarder::TcpSocket<sockaddr_in>;
template class DnsForwarder::TcpSocket<sockaddr_in6>;
template class DnsForwarder::TcpClient<sockaddr_in>;
template class DnsForwarder::TcpClient<sockaddr_in6>;
template class DnsForwarder::TcpServer<sockaddr_in>;
template class DnsForwarder::TcpServer<sockaddr_in6>;
template class DnsForwarder::TcpListener<sockaddr_in>;
template class DnsForwarder::TcpListener<sockaddr_in6>;