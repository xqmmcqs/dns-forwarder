#include "event.h"

#include <cstring>

#include <string>

#include "utils.h"

using namespace std;

template <typename AddrT> void DnsForwarder::UdpServerRecvEvent<AddrT>::Handler()
{
    auto logger = Logger::GetInstance();
    AddrT addr;
    string data;
    m_udp_server.ReceiveFrom(addr, data);
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Received UDP DNS request:\n" + Logger::RawDataFormatter(data));

    DnsPacket packet;
    istringstream is(data);
    packet.parse(is);

    auto index = m_task_pool.PutTask(make_shared<UdpTask>(packet, addr));
    packet.header.id = index;
    ostringstream os;
    packet.serialize(os);

    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Send UDP DNS request:\n" + Logger::RawDataFormatter(os.str()));
    for (const auto &remote_addr : m_remote_addr4)
    {
        if (m_udp_client4.SendTo(remote_addr, os.str()))
        {
            epoll_event event;
            event.data.ptr = &m_udp_client4;
            event.events = EPOLLIN | EPOLLET | EPOLLOUT;
            Wrapper::EpollModifyFd(m_epollfd, m_udp_client4.fd(), event);
        }
    }
    for (const auto &remote_addr : m_remote_addr6)
    {
        if (m_udp_client6.SendTo(remote_addr, os.str()))
        {
            epoll_event event;
            event.data.ptr = &m_udp_client6;
            event.events = EPOLLIN | EPOLLET | EPOLLOUT;
            Wrapper::EpollModifyFd(m_epollfd, m_udp_client6.fd(), event);
        }
    }
}

template <typename AddrT> void DnsForwarder::UdpClientRecvEvent<AddrT>::Handler()
{
    auto logger = Logger::GetInstance();
    AddrT addr;
    string data;
    m_udp_client.ReceiveFrom(addr, data);
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Received UDP DNS response:\n" + Logger::RawDataFormatter(data));

    DnsPacket packet;
    istringstream is(data);
    packet.parse(is);

    if (!m_task_pool.HasTask(packet.header.id))
        return;
    auto task_ptr = m_task_pool.GetTask(packet.header.id);
    if (packet.questions != task_ptr->query_packet.questions)
        return;
    m_task_pool.DelTask(packet.header.id);
    packet.header.id = task_ptr->query_packet.header.id;
    ostringstream os;
    packet.serialize(os);

    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Send UDP DNS response:\n" + Logger::RawDataFormatter(os.str()));
    if (task_ptr->is_ipv6)
    {
        if (m_udp_server6.SendTo(task_ptr->addr6, os.str()))
        {
            epoll_event event;
            event.data.ptr = &m_udp_server6;
            event.events = EPOLLIN | EPOLLET | EPOLLOUT;
            Wrapper::EpollModifyFd(m_epollfd, m_udp_server6.fd(), event);
        }
    }
    else
    {
        if (m_udp_server4.SendTo(task_ptr->addr, os.str()))
        {
            epoll_event event;
            event.data.ptr = &m_udp_server4;
            event.events = EPOLLIN | EPOLLET | EPOLLOUT;
            Wrapper::EpollModifyFd(m_epollfd, m_udp_server4.fd(), event);
        }
    }
}

template <typename AddrT> void DnsForwarder::TcpServerAcceptEvent<AddrT>::Handler()
{
    auto logger = Logger::GetInstance();
    AddrT addr;
    auto fd = m_tcp_listener.Accept(addr);
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Accepted TCP connection.");
    auto tcp_server = new TcpServer<AddrT>(fd);
    m_tcp_server.insert(tcp_server);
    epoll_event event;
    event.data.ptr = tcp_server;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    Wrapper::EpollAddFd(m_epollfd, fd, event);
}

template <typename AddrT> void DnsForwarder::TcpServerRecvEvent<AddrT>::Handler()
{
    auto logger = Logger::GetInstance();
    string data;
    m_tcp_server->Receive(data);
    if (data.size() == 0)
        return;
    size_t n = 0;
    while (n < data.size())
    {
        size_t len = data[n] * 256 + data[n + 1];
        string packet_data = data.substr(n + 2, len);
        n += len + 2;
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Received TCP DNS request:\n" + Logger::RawDataFormatter(packet_data));

        DnsPacket packet;
        istringstream is(packet_data);
        packet.parse(is);

        auto index = m_task_pool.PutTask(make_shared<TcpTask>(packet, m_tcp_server));
        packet.header.id = index;
        ostringstream os;
        packet.serialize(os);

        logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Send TCP DNS request:\n" + Logger::RawDataFormatter(os.str()));
        for (const auto &tcp_client : m_tcp_client4)
        {
            if (tcp_client->Send(string({len / 256, len % 256}) + os.str()))
            {
                epoll_event event;
                event.data.ptr = tcp_client;
                event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP;
                Wrapper::EpollModifyFd(m_epollfd, tcp_client->fd(), event);
            }
        }
        for (const auto &tcp_client : m_tcp_client6)
        {
            if (tcp_client->Send(string({len / 256, len % 256}) + os.str()))
            {
                epoll_event event;
                event.data.ptr = tcp_client;
                event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP;
                Wrapper::EpollModifyFd(m_epollfd, tcp_client->fd(), event);
            }
        }
    }
}

template <typename AddrT> void DnsForwarder::TcpClientRecvEvent<AddrT>::Handler()
{
    auto logger = Logger::GetInstance();
    string data;
    m_tcp_client->Receive(data);
    if (data.size() == 0)
        return;
    size_t n = 0;
    while (n < data.size())
    {
        size_t len = data[n] * 256 + data[n + 1];
        string packet_data = data.substr(n + 2, len);
        n += len + 2;
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Received TCP DNS response:\n" + Logger::RawDataFormatter(packet_data));

        DnsPacket packet;
        istringstream is(packet_data);
        packet.parse(is);

        if (!m_task_pool.HasTask(packet.header.id))
            return;
        auto task_ptr = m_task_pool.GetTask(packet.header.id);
        if (packet.questions != task_ptr->query_packet.questions)
            return;
        m_task_pool.DelTask(packet.header.id);
        packet.header.id = task_ptr->query_packet.header.id;
        ostringstream os;
        packet.serialize(os);

        logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Send TCP DNS response:\n" + Logger::RawDataFormatter(os.str()));
        if (task_ptr->is_ipv6)
        {
            if (m_tcp_server6.find(task_ptr->tcp_server6) != m_tcp_server6.end())
            {
                if (task_ptr->tcp_server6->Send(string({len / 256, len % 256}) + os.str()))
                {
                    epoll_event event;
                    event.data.ptr = task_ptr->tcp_server6;
                    event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP;
                    Wrapper::EpollModifyFd(m_epollfd, task_ptr->tcp_server6->fd(), event);
                }
            }
        }
        else
        {
            if (m_tcp_server4.find(task_ptr->tcp_server4) != m_tcp_server4.end())
            {
                if (task_ptr->tcp_server4->Send(string({len / 256, len % 256}) + os.str()))
                {
                    epoll_event event;
                    event.data.ptr = task_ptr->tcp_server4;
                    event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP;
                    Wrapper::EpollModifyFd(m_epollfd, task_ptr->tcp_server4->fd(), event);
                }
            }
        }
    }
}

template <typename AddrT> void DnsForwarder::TcpServerCloseEvent<AddrT>::Handler()
{
    auto logger = Logger::GetInstance();
    Wrapper::EpollDelFd(m_epollfd, m_tcp_server->fd());
    m_tcp_server_set.erase(m_tcp_server);
    delete m_tcp_server;
}

template <typename AddrT> void DnsForwarder::TcpClientCloseEvent<AddrT>::Handler()
{
    auto logger = Logger::GetInstance();
    auto addr = m_tcp_client->addr();
    Wrapper::EpollDelFd(m_epollfd, m_tcp_client->fd());
    m_tcp_client_set.erase(m_tcp_client);
    delete m_tcp_client;

    m_tcp_client = new TcpClient<AddrT>(addr);
    m_tcp_client_set.insert(m_tcp_client);
    epoll_event event;
    event.data.ptr = m_tcp_client;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    Wrapper::EpollAddFd(m_epollfd, m_tcp_client->fd(), event);
}

template class DnsForwarder::UdpServerRecvEvent<sockaddr_in>;
template class DnsForwarder::UdpServerRecvEvent<sockaddr_in6>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in6>;
template class DnsForwarder::TcpServerAcceptEvent<sockaddr_in>;
template class DnsForwarder::TcpServerAcceptEvent<sockaddr_in6>;
template class DnsForwarder::TcpServerRecvEvent<sockaddr_in>;
template class DnsForwarder::TcpServerRecvEvent<sockaddr_in6>;
template class DnsForwarder::TcpClientRecvEvent<sockaddr_in>;
template class DnsForwarder::TcpClientRecvEvent<sockaddr_in6>;
template class DnsForwarder::TcpServerCloseEvent<sockaddr_in>;
template class DnsForwarder::TcpServerCloseEvent<sockaddr_in6>;
template class DnsForwarder::TcpClientCloseEvent<sockaddr_in>;
template class DnsForwarder::TcpClientCloseEvent<sockaddr_in6>;