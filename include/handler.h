#pragma once

#include <cstring>

#include <string>
#include <unordered_set>

#include "task.h"
#include "utils.h"

namespace DnsForwarder
{
template <typename AddrT>
void UdpServerRecvHandler(const UdpServer<AddrT> &udp_server, UdpClient4 &udp_client4, UdpClient6 &udp_client6,
                          const std::vector<sockaddr_in> &remote_addr4, const std::vector<sockaddr_in6> &remote_addr6,
                          const int &epollfd, TaskPool<UdpTask> &task_pool)
{
    using namespace std;
    auto logger = Logger::GetInstance();
    AddrT addr;
    string data;
    udp_server.ReceiveFrom(addr, data);
    logger.Log(__FILE__, __LINE__, Logger::DEBUG,
               "Received UDP DNS request from " + Logger::SocketFormatter(addr) + ":\n" +
                   Logger::RawDataFormatter(data));

    DnsPacket packet;
    istringstream is(data);
    packet.parse(is);

    auto index = task_pool.PutTask(make_shared<UdpTask>(packet, addr));
    packet.header.id = index;
    ostringstream os;
    packet.serialize(os);

    for (const auto &remote_addr : remote_addr4)
    {
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Send UDP DNS request to " + Logger::SocketFormatter(remote_addr) + ":\n" +
                       Logger::RawDataFormatter(os.str()));
        if (udp_client4.SendTo(remote_addr, os.str()))
            Wrapper::EpollModFd(epollfd, udp_client4.fd(), &udp_client4, EPOLLIN | EPOLLET | EPOLLOUT);
    }
    for (const auto &remote_addr : remote_addr6)
    {
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Send UDP DNS request to " + Logger::SocketFormatter(remote_addr) + ":\n" +
                       Logger::RawDataFormatter(os.str()));
        if (udp_client6.SendTo(remote_addr, os.str()))
            Wrapper::EpollModFd(epollfd, udp_client6.fd(), &udp_client6, EPOLLIN | EPOLLET | EPOLLOUT);
    }
}

template <typename AddrT>
void UdpClientRecvHandler(const UdpClient<AddrT> &udp_client, UdpServer4 &udp_server4, UdpServer6 &udp_server6,
                          const int &epollfd, TaskPool<UdpTask> &task_pool)
{
    using namespace std;
    auto logger = Logger::GetInstance();
    AddrT addr;
    string data;
    udp_client.ReceiveFrom(addr, data);
    logger.Log(__FILE__, __LINE__, Logger::DEBUG,
               "Received UDP DNS response from " + Logger::SocketFormatter(addr) + ":\n" +
                   Logger::RawDataFormatter(data));

    DnsPacket packet;
    istringstream is(data);
    packet.parse(is);

    if (!task_pool.HasTask(packet.header.id))
        return;
    auto task_ptr = task_pool.GetTask(packet.header.id);
    if (packet.questions != task_ptr->query_packet.questions)
        return;
    task_pool.DelTask(packet.header.id);
    packet.header.id = task_ptr->query_packet.header.id;
    ostringstream os;
    packet.serialize(os);

    if (task_ptr->is_ipv6)
    {
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Send UDP DNS response to " + Logger::SocketFormatter(task_ptr->addr6) + ":\n" +
                       Logger::RawDataFormatter(os.str()));
        if (udp_server6.SendTo(task_ptr->addr6, os.str()))
            Wrapper::EpollModFd(epollfd, udp_server6.fd(), &udp_server6, EPOLLIN | EPOLLET | EPOLLOUT);
    }
    else
    {
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Send UDP DNS response to " + Logger::SocketFormatter(task_ptr->addr) + ":\n" +
                       Logger::RawDataFormatter(os.str()));
        if (udp_server4.SendTo(task_ptr->addr, os.str()))
            Wrapper::EpollModFd(epollfd, udp_server4.fd(), &udp_server4, EPOLLIN | EPOLLET | EPOLLOUT);
    }
}

template <typename AddrT>
void TcpServerAcceptHandler(const TcpListener<AddrT> &tcp_listener,
                            std::unordered_set<TcpServer<AddrT> *> &tcp_server_set, std::shared_mutex &tcp_server_mutex,
                            const int &epollfd)
{
    using namespace std;
    auto logger = Logger::GetInstance();
    AddrT addr;
    auto fd = tcp_listener.Accept(addr);
    logger.Log(__FILE__, __LINE__, Logger::DEBUG,
               "Accepted TCP connection from " + Logger::SocketFormatter(addr) + " on fd " + to_string(fd) + ".");
    auto tcp_server = new TcpServer<AddrT>(fd);
    unique_lock<shared_mutex> lock(tcp_server_mutex);
    tcp_server_set.insert(tcp_server);
    Wrapper::EpollAddFd(epollfd, fd, tcp_server, EPOLLIN | EPOLLET | EPOLLRDHUP);
}

template <typename AddrT>
void TcpServerRecvHandler(TcpServer<AddrT> *tcp_server, const std::unordered_set<TcpClient4 *> &tcp_client4,
                          const std::unordered_set<TcpClient6 *> &tcp_client6, std::shared_mutex &tcp_client4_mutex,
                          std::shared_mutex &tcp_client6_mutex, const int &epollfd, TaskPool<TcpTask> &task_pool)
{
    using namespace std;
    auto logger = Logger::GetInstance();
    string data;
    tcp_server->Receive(data);
    if (data.size() == 0)
        return;
    size_t n = 0;
    while (n < data.size())
    {
        size_t len = static_cast<uint8_t>(data[n]) * 256 + static_cast<uint8_t>(data[n + 1]);
        string packet_data = data.substr(n + 2, len);
        n += len + 2;
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Received TCP DNS request on fd " + to_string(tcp_server->fd()) + ":\n" +
                       Logger::RawDataFormatter(packet_data));

        DnsPacket packet;
        istringstream is(packet_data);
        packet.parse(is);

        auto index = task_pool.PutTask(make_shared<TcpTask>(packet, tcp_server));
        packet.header.id = index;
        ostringstream os;
        packet.serialize(os);

        shared_lock<shared_mutex> lock(tcp_client4_mutex);
        for (const auto &tcp_client : tcp_client4)
        {
            logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                       "Send TCP DNS request on fd " + to_string(tcp_client->fd()) + ":\n" +
                           Logger::RawDataFormatter(os.str()));
            if (tcp_client->Send(string({static_cast<char>(len / 256), static_cast<char>(len % 256)}) + os.str()))
                Wrapper::EpollModFd(epollfd, tcp_client->fd(), tcp_client, EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);
        }
        lock.unlock();
        lock = shared_lock<shared_mutex>(tcp_client6_mutex);
        for (const auto &tcp_client : tcp_client6)
        {
            logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                       "Send TCP DNS request on fd " + to_string(tcp_client->fd()) + ":\n" +
                           Logger::RawDataFormatter(os.str()));
            if (tcp_client->Send(string({static_cast<char>(len / 256), static_cast<char>(len % 256)}) + os.str()))
                Wrapper::EpollModFd(epollfd, tcp_client->fd(), tcp_client, EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);
        }
    }
}

template <typename AddrT>
void TcpClientRecvHandler(TcpClient<AddrT> *tcp_client, const std::unordered_set<TcpServer4 *> &tcp_server4,
                          const std::unordered_set<TcpServer6 *> &tcp_server6, std::shared_mutex &tcp_server4_mutex,
                          std::shared_mutex &tcp_server6_mutex, const int &epollfd, TaskPool<TcpTask> &task_pool)
{
    using namespace std;
    auto logger = Logger::GetInstance();
    string data;
    tcp_client->Receive(data);
    if (data.size() == 0)
        return;
    size_t n = 0;
    while (n < data.size())
    {
        size_t len = static_cast<uint8_t>(data[n]) * 256 + static_cast<uint8_t>(data[n + 1]);
        string packet_data = data.substr(n + 2, len);
        n += len + 2;
        logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                   "Received TCP DNS response on fd " + to_string(tcp_client->fd()) + ":\n" +
                       Logger::RawDataFormatter(packet_data));

        DnsPacket packet;
        istringstream is(packet_data);
        packet.parse(is);

        if (!task_pool.HasTask(packet.header.id))
            return;
        auto task_ptr = task_pool.GetTask(packet.header.id);
        if (packet.questions != task_ptr->query_packet.questions)
            return;
        task_pool.DelTask(packet.header.id);
        packet.header.id = task_ptr->query_packet.header.id;
        ostringstream os;
        packet.serialize(os);

        if (task_ptr->is_ipv6)
        {
            logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                       "Send TCP DNS response on fd " + to_string(task_ptr->tcp_server6->fd()) + ":\n" +
                           Logger::RawDataFormatter(os.str()));
            shared_lock<shared_mutex> lock(tcp_server6_mutex);
            if (tcp_server6.find(task_ptr->tcp_server6) != tcp_server6.end())
            {
                if (task_ptr->tcp_server6->Send(string({static_cast<char>(len / 256), static_cast<char>(len % 256)}) +
                                                os.str()))
                    Wrapper::EpollModFd(epollfd, task_ptr->tcp_server6->fd(), task_ptr->tcp_server6,
                                        EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);
            }
        }
        else
        {
            logger.Log(__FILE__, __LINE__, Logger::DEBUG,
                       "Send TCP DNS response on fd " + to_string(task_ptr->tcp_server4->fd()) + ":\n" +
                           Logger::RawDataFormatter(os.str()));
            shared_lock<shared_mutex> lock(tcp_server4_mutex);
            if (tcp_server4.find(task_ptr->tcp_server4) != tcp_server4.end())
            {
                if (task_ptr->tcp_server4->Send(string({static_cast<char>(len / 256), static_cast<char>(len % 256)}) +
                                                os.str()))
                    Wrapper::EpollModFd(epollfd, task_ptr->tcp_server4->fd(), task_ptr->tcp_server4,
                                        EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);
            }
        }
    }
}

template <typename AddrT>
void TcpServerCloseHandler(TcpServer<AddrT> *tcp_server, std::unordered_set<TcpServer<AddrT> *> &tcp_server_set,
                           std::shared_mutex &tcp_server_mutex, const int &epollfd)
{
    using namespace std;
    auto logger = Logger::GetInstance();
    Wrapper::EpollDelFd(epollfd, tcp_server->fd());
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Closed TCP server on fd " + to_string(tcp_server->fd()) + ".");
    unique_lock<shared_mutex> lock(tcp_server_mutex);
    tcp_server_set.erase(tcp_server);
    delete tcp_server;
}

template <typename AddrT>
void TcpClientCloseHandler(TcpClient<AddrT> *tcp_client, std::unordered_set<TcpClient<AddrT> *> &tcp_client_set,
                           std::shared_mutex &tcp_client_mutex, const int &epollfd)
{
    using namespace std;
    auto logger = Logger::GetInstance();
    auto addr = tcp_client->addr();
    Wrapper::EpollDelFd(epollfd, tcp_client->fd());
    unique_lock<shared_mutex> lock(tcp_client_mutex);
    tcp_client_set.erase(tcp_client);
    delete tcp_client;
    logger.Log(__FILE__, __LINE__, Logger::DEBUG, "Restart TCP client on fd " + to_string(tcp_client->fd()) + ".");

    tcp_client = new TcpClient<AddrT>(addr);
    tcp_client_set.insert(tcp_client);
    Wrapper::EpollAddFd(epollfd, tcp_client->fd(), tcp_client, EPOLLIN | EPOLLET | EPOLLRDHUP);
}
} // namespace DnsForwarder