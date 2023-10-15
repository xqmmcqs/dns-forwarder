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
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Received DNS packet from local:\n" + Logger::RawDataFormatter(data));

    DnsPacket packet;
    istringstream is(data);
    packet.parse(is);

    auto index = m_task_pool.PutTask(make_shared<UdpTask>(packet, addr));
    packet.header.id = index;
    ostringstream os;
    packet.serialize(os);
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Send DNS packet to remote:\n" + Logger::RawDataFormatter(os.str()));
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
    logger.Log(__FILE__, __LINE__, Logger::DEBUG,
               "Received DNS packet from remote:\n" + Logger::RawDataFormatter(data));

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
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Send DNS packet to local:\n" + Logger::RawDataFormatter(os.str()));
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

template class DnsForwarder::UdpServerRecvEvent<sockaddr_in>;
template class DnsForwarder::UdpServerRecvEvent<sockaddr_in6>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in6>;