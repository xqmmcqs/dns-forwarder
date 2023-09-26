#include "event.h"
#include <string>

using namespace std;

template <typename ServerAddrT, typename ClientAddrT>
void DnsForwarder::UdpServerRecvEvent<ServerAddrT, ClientAddrT>::Handler()
{
    ServerAddrT addr;
    string data;
    m_udp_server.ReceiveFrom(addr, data);

    DnsPacket packet;
    istringstream is(data);
    packet.parse(is);

    auto index = m_task_pool.PutTask(make_shared<UdpTask<ServerAddrT>>(packet, addr));
    packet.header.id = index;
    ostringstream os;
    packet.serialize(os);
    m_udp_client.SendTo(m_remote_addr, os.str());
}

template <typename ServerAddrT, typename ClientAddrT>
void DnsForwarder::UdpClientRecvEvent<ServerAddrT, ClientAddrT>::Handler()
{
    ClientAddrT addr;
    string data;
    m_udp_client.ReceiveFrom(addr, data);

    DnsPacket packet;
    istringstream is(data);
    packet.parse(is);

    auto task_ptr = m_task_pool.GetTask(packet.header.id);
    packet.header.id = task_ptr->query_packet.header.id;
    ostringstream os;
    packet.serialize(os);
    m_udp_server.SendTo(task_ptr->addr, os.str());
}

template class DnsForwarder::UdpServerRecvEvent<sockaddr_in, sockaddr_in>;
template class DnsForwarder::UdpServerRecvEvent<sockaddr_in, sockaddr_in6>;
template class DnsForwarder::UdpServerRecvEvent<sockaddr_in6, sockaddr_in>;
template class DnsForwarder::UdpServerRecvEvent<sockaddr_in6, sockaddr_in6>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in, sockaddr_in>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in, sockaddr_in6>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in6, sockaddr_in>;
template class DnsForwarder::UdpClientRecvEvent<sockaddr_in6, sockaddr_in6>;